
OBJS += techlibs/tritoncore/synth_tritoncore.o
OBJS += techlibs/tritoncore/tritoncore_ffssr.o
OBJS += techlibs/tritoncore/tritoncore_ffinit.o
OBJS += techlibs/tritoncore/tritoncore_opt.o



#$(eval $(call add_share_file,share/tritoncore,techlibs/tritoncore/arith_map.v))
$(eval $(call add_share_file,share/tritoncore,techlibs/tritoncore/cells_map.v))
$(eval $(call add_share_file,share/tritoncore,techlibs/tritoncore/cells_sim.v))





