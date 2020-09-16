/* -*- c++ -*- */
/*
 * Copyright 2004,2010-2012,2014 Free Software Foundation, Inc.
 *
 * This file is originally part of GNU Radio, and was copied
 * with modifications into gr-talir.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clock_recovery_mm_ext_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include <stdexcept>

namespace gr {
namespace talir {

clock_recovery_mm_ext_ff::sptr clock_recovery_mm_ext_ff::make(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit, int extra_channels)
{
    return gnuradio::get_initial_sptr(new clock_recovery_mm_ext_ff_impl(
        omega, gain_omega, mu, gain_mu, omega_relative_limit, extra_channels));
}

clock_recovery_mm_ext_ff_impl::clock_recovery_mm_ext_ff_impl(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit,
    int extra_channels)
    : block("clock_recovery_mm_ext_ff",
            io_signature::make(1 + extra_channels, 1 + extra_channels, sizeof(float)),
            io_signature::make(1 + extra_channels, 1 + extra_channels, sizeof(float))),
      d_mu(mu),
      d_gain_mu(gain_mu),
      d_gain_omega(gain_omega),
      d_omega_relative_limit(omega_relative_limit),
      d_last_sample(0),
      d_interp(new filter::mmse_fir_interpolator_ff()),
      d_extra_channels(extra_channels)
{
    if (omega < 1)
        throw std::out_of_range("clock rate must be > 0");
    if (gain_mu < 0 || gain_omega < 0)
        throw std::out_of_range("Gains must be non-negative");

    set_omega(omega); // also sets min and max omega
    set_relative_rate(1.0 / omega);
    enable_update_rate(true); // fixes tag propagation through variable rate block
    set_tag_propagation_policy(TPP_DONT);
}

clock_recovery_mm_ext_ff_impl::~clock_recovery_mm_ext_ff_impl() { delete d_interp; }

void clock_recovery_mm_ext_ff_impl::forecast(int noutput_items,
                                         gr_vector_int& ninput_items_required)
{
    unsigned ninputs = ninput_items_required.size();
    for (unsigned i = 0; i < ninputs; i++)
        ninput_items_required[i] =
            (int)ceil((noutput_items * d_omega) + d_interp->ntaps());
}

void clock_recovery_mm_ext_ff_impl::set_omega(float omega)
{
    d_omega = omega;
    d_omega_mid = omega;
    d_omega_lim = d_omega_mid * d_omega_relative_limit;
}

int clock_recovery_mm_ext_ff_impl::general_work(int noutput_items,
                                            gr_vector_int& ninput_items,
                                            gr_vector_const_void_star& input_items,
                                            gr_vector_void_star& output_items)
{
    const float* in = (const float*)input_items[0];
    float* out = (float*)output_items[0];

    int ii = 0;                       // input index
    int oo = 0;                       // output index
    int ntaps = d_interp->ntaps();
    int ni = ninput_items[0] - ntaps; // don't use more input than this
    float mm_val;

    uint64_t in_abs_nitems = nitems_read(0);
    uint64_t out_abs_nitems = nitems_written(0);

    std::vector<tag_t> tags;
    get_tags_in_window(tags, 0, 0, ni);
    while (!tags.empty() && tags[0].offset < in_abs_nitems + ntaps/2)
        tags.erase(tags.begin());

    while (oo < noutput_items && ii < ni) {
        // produce output sample
        out[oo] = d_interp->interpolate(&in[ii], d_mu);

        // produce samples on extra channels
        for (int i = 1; i < 1 + d_extra_channels; i++) {
            float_t *chan_in = (float *) input_items[i];
            float_t *chan_out = (float *) output_items[i];
            chan_out[oo] = d_interp->interpolate(&chan_in[ii], d_mu);
        }

        // pass tags
        while (!tags.empty() && tags[0].offset <= in_abs_nitems + ii + ntaps/2) {
            tag_t &head = tags[0];
            head.offset = out_abs_nitems + oo;
            add_item_tag(0, head);
            tags.erase(tags.begin());
        }

        mm_val = slice(d_last_sample) * out[oo] - slice(out[oo]) * d_last_sample;
        d_last_sample = out[oo];

        d_omega = d_omega + d_gain_omega * mm_val;
        d_omega = d_omega_mid + gr::branchless_clip(d_omega - d_omega_mid, d_omega_lim);
        d_mu = d_mu + d_omega + d_gain_mu * mm_val;

        ii += (int)floor(d_mu);
        d_mu = d_mu - floor(d_mu);
        oo++;
    }

    consume_each(ii);
    return oo;
}

} /* namespace talir */
} /* namespace gr */
