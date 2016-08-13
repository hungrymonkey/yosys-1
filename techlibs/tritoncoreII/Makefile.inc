
OBJS += techlibs/tritoncoreII/synth_tritoncore2.o
#OBJS += techlibs/tritoncoreII/tritoncore_opt.o



#$(eval $(call add_share_file,share/tritoncoreII,techlibs/tritoncoreII/arith_map.v))
$(eval $(call add_share_file,share/tritoncoreII,techlibs/tritoncoreII/cells_map.v))





