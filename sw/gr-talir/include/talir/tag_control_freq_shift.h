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


#ifndef INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_H
#define INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_H

#include <talir/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace talir {

    /*!
     * \brief <+description of block+>
     * \ingroup talir
     *
     */
    class TALIR_API tag_control_freq_shift : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<tag_control_freq_shift> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of talir::tag_control_freq_shift.
       *
       * To avoid accidental use of raw pointers, talir::tag_control_freq_shift's
       * constructor is in a private implementation
       * class. talir::tag_control_freq_shift::make is the public interface for
       * creating new instances.
       */
      static sptr make(float samp_rate);
    };

  } // namespace talir
} // namespace gr

#endif /* INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_H */

