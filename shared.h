/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * Modified to implement an encoder/decoder client-server
 * Alexander Potashev <aspotashev@gmail.com>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once


namespace Opcode {
enum Opcodes {
  func_encode, func_decode
};
};

namespace Protocol {
enum Protocols {
  Encoder
};
};
