# gr-talir

`gr-talir` contains GNU Radio blocks written for signal processing in TALIR01 operations.

## Blocks

 * `clock_recovery_mm_ext_ff`: An extension of the "Clock Recovery MM" block distributed with GNU Radio. Includes support for auxiliary channels that undergo resampling directed by clock recovery on the main channel. Is limited to real channels.

 * `tag_control_freq_shift`: Carries out frequency shift directed by tags in the incoming stream.

 * `inject_msg_as_tag_cc`: Injects messages from message port as tags into a stream. The messages must be pairs, their car is key and their cdr is value of the created tag.
