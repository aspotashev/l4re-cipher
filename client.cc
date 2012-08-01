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
#include <l4/re/dataspace>      // L4Re::Dataspace
#include <l4/re/rm>             // L4::Rm
#include <l4/sys/cache.h>
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
	L4::Cap<void> const &server, char *shm_addr,
	char *result, const char *input, l4_uint32_t length, l4_uint32_t reverse)
{
  // <--- Ruby-style indentation - wat?

  L4::Ipc::Iostream s(l4_utcb());
  s << l4_umword_t(reverse ? Opcode::func_decode : Opcode::func_encode) << length;

  memcpy(shm_addr, input, length);
  l4_cache_clean_data((unsigned long)shm_addr, (unsigned long)shm_addr + length);

  l4_msgtag_t res = s.call(server.cap());
  if (l4_ipc_error(res, l4_utcb()))
    return 1; // failure

  l4_cache_clean_data((unsigned long)shm_addr, (unsigned long)shm_addr + length);
  memcpy(result, shm_addr, length);

  return 0; // ok
}

int func_getbuf_call(L4::Cap<void> const &server, L4::Cap<L4Re::Dataspace> const &ds)
{
  L4::Ipc::Iostream s(l4_utcb());
  s << l4_umword_t(Opcode::func_getbuf);

  // Thanks, Doxygen!
  // http://os.inf.tu-dresden.de/L4Re/doc/classL4_1_1Ipc_1_1Small__buf.html
  s << L4::Ipc::Small_buf(ds);

  l4_msgtag_t res = s.call(server.cap());
  if (l4_ipc_error(res, l4_utcb()))
    return 1; // failure

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

  /*
   * Alloc data space cap slot
   */
  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds.is_valid())
    {
      printf("Could not get capability slot!\n");
      return 1;
    }

  if (func_getbuf_call(server, ds))
  {
      printf("func_getbuf_call() failed\n");
      return 1;
  }

  /*
   * Attach to arbitrary region
   */
  char *shm_addr = 0;
  int err = L4Re::Env::env()->rm()->attach(&shm_addr, ds->size(),
                                           L4Re::Rm::Search_addr, ds);
  if (err < 0)
    {
      printf("Error attaching data space: %s\n", l4sys_errtostr(err));
      return 1;
    }


  const char *input = "Lodnon 2102 Oimplycs"; // go, KFC!
  int len = strlen(input);

  char *encoded = (char *)malloc(len + 1);
  char *decoded = (char *)malloc(len + 1);

  printf("Encoding \"%s\"...\n", input);
  if (func_encode_call(server, shm_addr, encoded, input, len, 0))
    {
      printf("Error talking to server\n");
      return 1;
    }
  printf("Encoded \"%s\" as \"%s\".\n", input, encoded);

  printf("Decoding \"%s\"...\n", encoded);
  if (func_encode_call(server, shm_addr, decoded, encoded, len, 1))
    {
      printf("Error talking to server\n");
      return 1;
    }
  printf("Decoded \"%s\" as \"%s\".\n", encoded, decoded);

  free(decoded);
  free(encoded);

  /*
   * Detach region containing addr, result should be Detached_ds (other results
   * only apply if we split regions etc.).
   */
  err = L4Re::Env::env()->rm()->detach(shm_addr, 0);
  if (err)
    printf("Failed to detach region\n");

  L4Re::Util::cap_alloc.free(ds, L4Re::This_task);

  return 0;
}
