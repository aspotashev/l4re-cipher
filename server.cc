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
#include <string.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/dataspace>
#include <l4/cxx/ipc_server>

#include "shared.h"

static L4Re::Util::Registry_server<> server;

enum
{
  DS_SIZE = 4 << 12,
};

static char *get_ds(L4::Cap<L4Re::Dataspace> *_ds)
{
  *_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!(*_ds).is_valid())
    {
      printf("Dataspace allocation failed.\n");
      return 0;
    }

  int err =  L4Re::Env::env()->mem_alloc()->alloc(DS_SIZE, *_ds, 0);
  if (err < 0)
    {
      printf("mem_alloc->alloc() failed.\n");
      L4Re::Util::cap_alloc.free(*_ds);
      return 0;
    }

  /*
   * Attach DS to local address space
   */
  char *_addr = 0;
  err =  L4Re::Env::env()->rm()->attach(&_addr, (*_ds)->size(),
                                        L4Re::Rm::Search_addr,
                                        *_ds);
  if (err < 0)
    {
      printf("Error attaching data space: %s\n", l4sys_errtostr(err));
      L4Re::Util::cap_alloc.free(*_ds);
      return 0;
    }


  /*
   * Success! Write something to DS.
   */
  printf("Attached DS\n");
  static char const * const msg = "[DS] Hello from server!";
  snprintf(_addr, strlen(msg) + 1, msg);

  return _addr;
}

static L4::Cap<L4Re::Dataspace> ds;

class Encoding_server : public L4::Server_object
{
public:
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);
};

static char *shm_addr;

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
        if (opcode == Opcode::func_encode)
          shm_addr[i] += 3; // Caesar "cipher"
        else
          shm_addr[i] -= 3;
      }

      return L4_EOK;
    }
    case Opcode::func_getbuf:
      ios << ds;
      return L4_EOK;
    default:
      return -L4_ENOSYS;
    }
}

int
main()
{
  static Encoding_server enc;

  if (!(shm_addr = get_ds(&ds)))
  {
    printf("get_ds() failed\n");
    return 7;
  }

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
