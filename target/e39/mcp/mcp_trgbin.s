.global mcp_loader
.global mcp_loaderSize

.global mcp_bin
.global mcp_binSize

mcp_loader:
.incbin "target/bin/loader.bin"
mcp_loaderSize:    .long  (mcp_loaderSize - mcp_loader)


mcp_bin:
.incbin "compbin.bin"
mcp_binSize:    .long  (mcp_binSize - mcp_bin)








# Last line to please the whining assembler
