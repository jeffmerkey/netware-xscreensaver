# xscreensaver(1) - Novell Netware SMP console screensaver

X Version 11, 27-Apr-97

```
netwaresmp [-cpus  number of cpus] [-speedup  divisor] [-wormsize  pixels] [-display host:display.screen] [-foreground color] [-background color] [-window] [-root] [-mono] [-install] [-visual visual] [-delay seconds] [-max-depth int] [-min-width int] [-min-height int] [-cycle] [-no-cycle] [-cycle-delay]
```

<a name="description"></a>

# Description

The _netwaresmp_ screensaver written by Jeffrey Merkey &lt;jeffmerkey@gmail.com&gt; and Cosimo Streppone &lt;cosimo@cpan.org&gt;, based on PopSquares, Deco hacks, loadsnake, and Novell's Netware SMP.  The original Netware SMP screensaver was written by Jeffrey Merkey at Novell in 1994.  This version is a rewrite of the original which functions exactly the same way as it did in Netware SMP.  Each worm represents a running system cpu processor.  As system utilization increases for each cpu, the corresponding worm for that processor gets longer and moves more quicky across the screen.  As overall system load increases, all the worms slightly increase in speed.  This screensaver is very useful for monitoring cpu loading in a system since you can visually see processor utilization and loading by simply observing the worms. 

<a name="options"></a>

# Options

_netwaresmp_
accepts the following options:

* **-cpus**  
  Number of cpus to start.  The default is the actual number of cpus detected from /proc/cpuinfo.  If you specify more cpus than are installed in your system, only the worms that are mapped to an actual processor will grow longer and faster when cpu load increases for that processor.  If you specify less cpus than are installed in your system, the program will default the display to the actual detected cpus.  
* **-speedup**  
  speedup divisor to increase worm speed.  functions as powers of 2 i.e. speedup=4 runs at 4 times the speed, etc.  Default value is 1.
* **-wormsize**  
  size of the worm head in pixels in a range between 10 and 80.  Default value is 20.  Can be any value but works best if you use multiples of 2 to map to graphics screen dimensions (which are typically numbers which are multiples of 2 or 10).
* **-window**  
  Draw on a newly-created window.  This is the default.
* **-root**  
  Draw on the root window.
* **-mono**  
  If on a color display, pretend we're on a monochrome display.
* **-install**  
  Install a private colormap for the window.
* **-visual _visual_**  
  Specify which visual to use.  Legal values are the name of a visual class,
  or the id number (decimal or hex) of a specific visual.
* **-delay _seconds_**  
  How long to wait before starting over.  Default 5 seconds.
* **-max-depth _integer_**  
  How deep to subdivide.  Default 12.
* **-min-width _integer_**  
  **-min-height _integer_**
  The size of the smallest rectangle to draw.  Default 20x20.
* **-cycle**  
* **-no-cycle**  
  Whether to do color cycling.  Default False.
* **-cycle-delay _usecs_**  
  If color cycling, how often to change the colors.  Default 1000000,
  or 1 second.

<a name="environment"></a>

# Environment



* **DISPLAY**  
  to get the default host and display number.
* **XENVIRONMENT**  
  to get the name of a resource file that overrides the global resources
  stored in the RESOURCE_MANAGER property.

<a name="see-also"></a>

# See Also

**X**(1),
**xscreensaver**(1)

<a name="copyright"></a>

# Copyright

Copyright © 1994-2024 by Jamie Zawinski, Jeffrey Merkey, Cosimo Streppone, 
and others.  Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee, provided 
that the above copyright notice appear in all copies and that both that 
copyright notice and this permission notice appear in supporting documentation. No representations are made about the suitability of this software for any 
purpose.  It is provided "as is" without express or implied warranty.

<a name="authors"></a>

# Authors

Jeffrey Merkey &lt;[jeffmerkey@gmail.com](mailto:jeffmerkey@gmail.com)&gt; and Cosimo Streppone &lt;cosimo@cpan.org&gt;, 
based on PopSquares, Deco hacks, loadsnake, and Novell's Netware SMP.  The 
original Netware SMP screensaver was written by Jeffrey Merkey at Novell in
1994.  
