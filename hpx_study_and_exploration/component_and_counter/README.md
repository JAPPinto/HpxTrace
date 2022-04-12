* Compilation
  * `cd build`
  * `cmake ..`
  * `make`
  * `sudo mv libexample_counters.so  /usr/local/lib/hpx/libhpx_example_counters.so`  [see here](https://github.com/STEllAR-GROUP/hpx/issues/5216)


* api_name
  * Example that uses name_counter in the code
  * With name_counter the correspondent component is identified through its registered name
  * ./api_name

* api_basename
  * Example that uses basename_counter in the code
  *  With basename_counter the correspondent component is identified through its registered basename and sequence number
  * ./api_basename

* cmd
  * Example to use (base)name_counters with the command line
  * With name_counter the correspondent component is identified through its registered name 
  * ./cmd --hpx:print-counter=/examples{locality#0/component#0}/name/explicit --hpx:print-counter-interval=500
  * ./cmd --hpx:print-counter=/examples{locality#0/component#0}/basename/explicit --hpx:print-counter=/examples{locality#0/component#1}/basename/explicit  --hpx:print-counter-interval=500
