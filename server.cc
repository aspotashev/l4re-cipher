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
#include <stdio.h>
#include <stdlib.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/cxx/ipc_server>

#include "shared.h"

static L4Re::Util::Registry_server<> server;

class Encoding_server : public L4::Server_object
{
public:
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);
};

int
Encoding_server::dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
{
  l4_msgtag_t t;
  ios >> t;

  // We're only talking the encoding protocol
  if (t.label() != Protocol::Encoder)
    return -L4_EBADPROTO;

  L4::Opcode opcode;
  ios >> opcode;

  switch (opcode)
    {
    case Opcode::func_encode:
    case Opcode::func_decode:
    {
      l4_uint32_t len;
      ios >> len;

// Looks like we cannot use malloc() here, probably because it spoils UTCB.
//      l4_umword_t *tmp = (l4_umword_t *)malloc(sizeof(l4_umword_t) * len);

      for (l4_uint32_t i = 0; i < len; i++)
      {
        l4_umword_t tmp;
        ios >> tmp;

        if (opcode == Opcode::func_encode)
          tmp += 3 * 0x01010101; // Caesar "cipher"; ugly hack included
        else
          tmp -= 3 * 0x01010101;

        ios << tmp;
      }

      return L4_EOK;
    }
    default:
      return -L4_ENOSYS;
    }
}

int
main()
{
  static Encoding_server enc;

  // Register encoding server
  if (!server.registry()->register_obj(&enc, "enc_server").is_valid())
    {
      printf("Could not register my service, readonly namespace?\n");
      return 1;
    }

  printf("Welcome to the encoding server!\n"
         "I can encode and decode text strings.\n");

  // Wait for client requests
  server.loop();

  return 0;
}
