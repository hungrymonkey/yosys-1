
OBJS += techlibs/minifpga/synth_minifpga.o
OBJS += techlibs/minifpga/minifpga_ffssr.o
OBJS += techlibs/minifpga/minifpga_ffinit.o
OBJS += techlibs/minifpga/minifpga_opt.o



#$(eval $(call add_share_file,share/minifpga,techlibs/minifpga/arith_map.v))
$(eval $(call add_share_file,share/minifpga,techlibs/minifpga/cells_map.v))
$(eval $(call add_share_file,share/minifpga,techlibs/minifpga/cells_sim.v))





