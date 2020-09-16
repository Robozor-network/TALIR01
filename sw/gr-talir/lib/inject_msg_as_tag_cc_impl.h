/* -*- c++ -*- */
/* 
 * Copyright 2020 gr-talir author.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_TALIR_INJECT_MSG_AS_TAG_CC_IMPL_H
#define INCLUDED_TALIR_INJECT_MSG_AS_TAG_CC_IMPL_H

#include <deque>
#include <pmt/pmt.h>
#include <talir/inject_msg_as_tag_cc.h>

namespace gr {
  namespace talir {

    class inject_msg_as_tag_cc_impl : public inject_msg_as_tag_cc
    {
     private:
      void handle_msg(pmt::pmt_t msg);
      std::deque<pmt::pmt_t> qu;

     public:
      inject_msg_as_tag_cc_impl();
      ~inject_msg_as_tag_cc_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace talir
} // namespace gr

#endif /* INCLUDED_TALIR_INJECT_MSG_AS_TAG_CC_IMPL_H */

