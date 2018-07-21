# dnsserver及dnsmodifier守护器

## 用法

### 启动监控：DNSMonitor.exe a.exe b.exe c.exe

    此命令会启动两个DNSMonitor.exe程序自监控，以及a.exe b.exe c.exe

    注意：a.exe b.exe c.exe要么是与DNSMonitor.exe同级目录，要么系统目录能够找到。

### 停止监控：DNSMonitor.exe stop

    此命令停止之前传入的exe及自身
        