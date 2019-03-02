CXX=CL -EHsc -DNDEBUG -Fo$@ -O2
#CXX=g++ -ggdb -O2 -lm -std=c++11 -DNDEBUG 

ABSEIL_LIBS=absl_bad_optional_access.lib absl_bad_variant_access.lib absl_base.lib absl_demangle_internal.lib absl_dynamic_annotations.lib absl_failure_signal_handler.lib absl_hash.lib absl_int128.lib absl_internal_bad_any_cast_impl.lib absl_internal_city.lib absl_internal_civil_time.lib absl_internal_debugging_internal.lib absl_internal_examine_stack.lib absl_internal_graphcycles_internal.lib absl_internal_hashtablez_force_sampling.lib absl_internal_hashtablez_sampler.lib absl_internal_malloc_internal.lib absl_internal_spinlock_wait.lib absl_internal_str_format_internal.lib absl_internal_strings_internal.lib absl_internal_throw_delegate.lib absl_internal_time_zone.lib absl_leak_check.lib absl_leak_check_disable.lib absl_optional.lib absl_raw_hash_set.lib absl_stacktrace.lib absl_strings.lib absl_symbolize.lib absl_synchronization.lib absl_time.lib

PROGS       = stl_unordered_map abseil_flat abseil_parallel_flat
BUILD_PROGS = $(addprefix build/,$(PROGS))
SIZE        = 100000000

all: $(BUILD_PROGS)

build/stl_unordered_map: bench.cc Makefile
	$(CXX) -DSTL_UNORDERED bench.cc -o $@

build/abseil_flat: bench.cc Makefile
	$(CXX) -DABSEIL_FLAT -I ../abseil-cpp bench.cc /MD -o $@ /link /LIBPATH:../abseil-cpp/build2/lib ${ABSEIL_LIBS}

build/abseil_parallel_flat: bench.cc Makefile
	$(CXX) -DABSEIL_PARALLEL_FLAT -I ../abseil-cpp bench.cc /MD -o $@ /link /LIBPATH:../abseil-cpp/build2/lib ${ABSEIL_LIBS}


test:
	-rm -f output
	./build/stl_unordered_map $(SIZE) random > output
	./build/abseil_flat $(SIZE) random >> output
	./build/abseil_parallel_flat $(SIZE) random >> output





