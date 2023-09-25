.global mcp_loader
.global mcp_loaderSize

mcp_loader:
.incbin "target/bin/loader.bin"
mcp_loaderSize:    .long  (mcp_loaderSize - mcp_loader)
