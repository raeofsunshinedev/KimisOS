# Kernel Messages

Messages are one-way generic communication system from the kernel to modules. It's designed to support a first-come, first-serve approach to handling of tasks like filesystem mounting, while also supporting broadcast style messages (e.g. a shutdown), that applies to every module with a listener.

Modules are not required to provide a handler, nor support all types of messages. However, modules with handlers are expected to return `-1` in the event of a failure, or if the message dispatched is not supported.