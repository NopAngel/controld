![controld](assets/logo.png)

### controld

#### Minimalist System and Service Manager

`controld` is a lightweight process supervisor and init system for Linux, written in C. It manages the lifecycle of system services, handling everything from process spawning and journaling to advanced networking via Netlink sockets.

#### Details

- **Service Supervision**: Automatic spawning and monitoring of processes.

- **Dependency Management**: Support for the After= directive to ensure correct boot ordering.

- **Integrated Journaling**: Automatic redirection of stdout and stderr to individual logs in `/logs.`

- **Process Timers**: Support for scheduled execution of services.

- **Hot Reload**: Reload configuration files on-the-fly without stopping the main daemon.

- **Network Management**: Native support for setting interface states *(UP/DOWN)* and static IP assignment using Netlink Sockets.

- **IPC Control**: A dedicated client (**controlctl**) to communicate with the daemon via Unix Domain Sockets.

- **Graceful Shutdown**: Reliable signal handling to ensure all child processes are terminated cleanly.

#### LICENSE: GPLv3+