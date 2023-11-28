				World O' Networking


World O' Networking (WON) is the BeOS interface for sharing files and
services over a network. If you're on a LAN (with 2 or more computers),
when you open WON it automatically "populates" your WON window and
gives you access to:

* File sharing through CIFS 1.0 (Common Internet File System).
   This means Be users can now share files and resources
   on Windows machines, and Unix machines running SAMBA. 
   WON also supports encrypted password. 
* Printing for CIFS and AppleTalk. Because the print layer and
   print panels are now network aware, it's easy to add a default
   network printer.

To start WON:

1. Launch the CIFS setup app called World O' Networking Setup; you'll find
it in your optional/experimental directory.
2. In the setup panel, check File and Printer sharing, name your workgroup
(this is the name given to a group of machines), if your unsure just leave
WORKGROUP. For user name and password you should know what names 
your sys admin has used to setup the remote shares. Finally, you must
enter (and confirm) a password. In a typical shared network many resources
are actually shared but not shown to users, the checkbox Show hidden
mounts controls this feature. It is off by default.

NOTE: You may need additional permission if you try to share a group or
folder; this depends on what your sys admin has set up.

Once you've configured your WON with the above information, you can share
files and printers, and can browse the network. Access is through a desktop
icon called World O' Networking. Click on the icon and there should appear
a list of workgroups on the network, clicking on the workgroups should show
a list of machines, clicking on a machine will show files shares and printers. 
The layout of workgroups and machines depends on how the sys admin has 
configured the network. The windows showing the lists of workgroups and
machines can be closed, and if desired,  reopened by right clicking the WON 
icon.

File Sharing

You can drag and drop files to and from your system. Files you don't want
to share can be password protected. Among other uses, this is good for
backing up files onto another computer.

Network Printing

Launch WON and choose a printer from the WON window. WON is integrated
to BeOS Printer prefs, which pops up when you choose a printer. You can add a
printer (the panel is already set for Network printer), click OK, and then
configure your network printer in the next window.


Command line tools:
Included in the WON package are a couple tools that you might find 
helpful. Often they can be used to diagnose problems with WON, or
help understand how the sys admin has setup the network.

* cifsmount takes several parameters and will mount remote shares.
   It does not use NetBios names so make sure the server has
   a DNS entry or is locally listed in the hosts file.  See hosts-sample 
   on how how to do this. Some servers are picky about case, when
   in doubt use upper case. 

   Syntax:
   cifmount \\\\SOMESERVER\\ASHARE USERNAME PASSWORD LOCALMOUNT

    Sample:
    cifsmount \\\\tiger2\temp SAM SAMMY /mnt

* ezmount is just a simple script that wraps up cifsmount.

    Syntax:
    ezmount SOMESEVER ASHARE USERNAME PASSWORD

*  unmount will remove the local shared mount. Unmount is not a part of the
    WON package, it's the same command used for regular mounts.

    Syntax:
    unmount LOCALMOUNT   

WON Package:
Most of WON is integrated into the OS, however if needed it's easy
to turn off. Use World O' Networking Setup and uncheck File and Printer 
sharing. Or edit the /beos/system/boot/Netscript file, and remove the
following lines: 

start  beos/bin/ksocketd
start beos/system/servers/da_hood


Current Release Status (could change for R4.1):
The BeOS has only client capability. This means that you can
access files from other systems and move files from your system (that is,
you can read and write files), but a BeOS machine or volume can't act as a
server--it won't appear in your WON window as a server. You'll be able to
see only machines running Windows 95/98, Windows NT, and Unix (running
Samba); you'll also be able to see files and printers. We are looking at adding
this as an R4.1 feature. The goal is to port the Samba server to the BeOS, and
integrate with WON.



