# PSOD

A simple Windows driver which registers two IOCTL handlers, the first of which installs a bug check callback and the second triggers a bug check.   
The implemented bug check callback will modify the VGI settings in order to change the 'Blue Screen Of Deaths' colors from blue and white to pink and black.

![crersh](https://raw.githubusercontent.com/sam-b/PSOD/master/screenshots/psod.PNG)

More details at: (https://www.whitehatters.academy/my-first-windows-driver-creating-the-pink-screen-of-death/)[https://www.whitehatters.academy/my-first-windows-driver-creating-the-pink-screen-of-death/]