/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * Modified to implement an encoder/decoder client-server
 * Alexander Potashev <aspotashev@gmail.com>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/cxx/ipc_stream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"

// We assume the input length to be equal to the length of encoded data,
// i.e. @length is the size of both @result and @input.
//
// @dir: 0=encode; 1=decode
int func_encode_call(
	L4::Cap<void> const &server,
	char *result, const char *input, l4_uint32_t length, l4_uint32_t reverse)
{
  // <--- Ruby-style indentation - wat?

  L4::Ipc::Iostream s(l4_utcb());
  s << l4_umword_t(reverse ? Opcode::func_decode : Opcode::func_encode) << length;
  for (l4_uint32_t i = 0; i < length; i++)
    s << l4_umword_t(input[i]); // 4x overhead on an 32-bit arch, but who cares...

  l4_msgtag_t res = s.call(server.cap());
  if (l4_ipc_error(res, l4_utcb()))
    return 1; // failure

  for (l4_uint32_t i = 0; i < length; i++)
  { // <--- let's learn C++ coding style back
    l4_umword_t tmp;
    s >> tmp;
    result[i] = (char)tmp; // truncate machine word to 1 byte
  }
  return 0; // ok
}

int
main()
{

  L4::Cap<void> server = L4Re::Env::env()->get_cap<void>("enc_server");
  if (!server.is_valid())
    {
      printf("Could not get server capability!\n");
      return 1;
    }

  const char *input = "Lodnon 2102 Oimplycs"; // go, KFC!
  int len = strlen(input);

  char *encoded = (char *)malloc(len + 1);
  char *decoded = (char *)malloc(len + 1);

  printf("Encoding \"%s\"...\n", input);
  if (func_encode_call(server, encoded, input, len, 0))
    {
      printf("Error talking to server\n");
      return 1;
    }
  printf("Encoded \"%s\" as \"%s\".\n", input, encoded);

  printf("Decoding \"%s\"...\n", encoded);
  if (func_encode_call(server, decoded, encoded, len, 1))
    {
      printf("Error talking to server\n");
      return 1;
    }
  printf("Decoded \"%s\" as \"%s\".\n", encoded, decoded);

  free(decoded);
  free(encoded);
  return 0;
}
