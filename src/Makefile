CFLAGS = 
MPIHOME=./
CC = 
LIBS =  
INCLUDES =
OBJS = 
SRCS = 
INSTALL_DIR=tests/
INSTALL_IMB=IMB-3.2
INSTALL_PRESTA=presta-1.4.0
INSTALL_OSU=osu_benchmarks-3.1.1
all: ibm osu presta
install: install-ibm install-presta install-osu

# The variable $@ has the value of the target. 
ibm: 
	cd $(PWD)/$(INSTALL_IMB)/src && make MPIHOME=$(MPIHOME) IMB-MPI1
osu: 
	cd $(PWD)/$(INSTALL_OSU) && make MPIHOME=$(MPIHOME)
presta:
	cd $(PWD)/$(INSTALL_PRESTA) && make MPIHOME=$(MPIHOME)
clean-ibm: 
	cd $(PWD)/$(INSTALL_IMB)/src && make MPIHOME=$(MPIHOME) clean
clean-osu: 
	cd $(PWD)/$(INSTALL_OSU) && make MPIHOME=$(MPIHOME) clean
clean-presta:
	cd $(PWD)/$(INSTALL_PRESTA) && make MPIHOME=$(MPIHOME) clean
clean: clean-ibm clean-presta clean-osu 
	rm -rf $(MPIHOME)/$(INSTALL_DIR)
install-ibm:
	mkdir -p $(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_IMB)
	cp -f $(INSTALL_IMB)/src/IMB-MPI1 $(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_IMB)
install-presta:
	mkdir -p $(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_PRESTA)
	cp -f $(INSTALL_PRESTA)/com    		$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_PRESTA)
	cp -f $(INSTALL_PRESTA)/glob     	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_PRESTA)
	cp -f $(INSTALL_PRESTA)/globalop 	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_PRESTA)
install-osu:
	mkdir -p $(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_bw		    $(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_bibw		$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_bcast		$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_latency	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_mbw_mr  	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_alltoall  	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
	cp -f $(INSTALL_OSU)/osu_multi_lat	$(MPIHOME)/$(INSTALL_DIR)/$(INSTALL_OSU)
