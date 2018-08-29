SUBDIRS := db_client utility netlisten webcmd serverapp qiniu
all:
	$(foreach N,$(SUBDIRS),make -C $(N);)
clean:
	$(foreach N,$(SUBDIRS),make -C $(N) clean;)
