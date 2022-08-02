Camera Image Recieve Test Utility V3.1
======================================

NEW Modifications.
=================
Friday 15 March 2019 12:29 pm

Application to recieve images streamed by Multiple RLVD camera.

No more Arguments to application.
Please Make a file for the config structure given in main for the purpose ad give file as an argument.
This will be easier for your deployment and testing.

struct ipconfigs
{
  int	valid;
  char	ipaddr[18];
  char	port[5];
};



Friday 16 November 2018 12:49 pm
--------------------------------------
Improved threaded program.. to optimise performance..

Thursday 27 December 2018 01:23 pm
--------------------------------------
FOR RLVD/ANPR 

Automatic image numbering of images with channel info.
channel 0 = evidence from TK1
channel 1 = lane 0
channel 2 = lane 1. etc

The image files are saved in /dev/shm/Images
please create "Images" directory in /dev/shm

The file name consists of image capture info. like High/Low light, channel no and sequence no.

L00img0000001.jpg
L=low
00=channel 0 = evidence
img000001 sequnce no.

