##About AutoGUI  
***  
AutoGUI is designed to capture and replay VNC streams for research purposes. The captured VNC streams can also be replayed on the [gem5 simulator](http://www.m5sim.org).  

The basic feature of AutoGUI is recording and forwarding the messages data between vnc-client and vnc-server.  
Since AutoGUI is still under developing, you are welcomed to enhance the implementation and feel free to submit your code!  


##User Guide    
***  
####Prerequisite  

a. Any Linux kernel and disk image for Android which can used in gem5 simulator.   
	If you don't have any available image in-hand, the [Moby](http://asg.ict.ac.cn/projects/moby/), which is mobile benchmark suite, maybe is a good option.  

b. Any Linux kernel and disk image for Android which are used in QEMU.  
	Of course, you can use the same disk image for both gem5 and QEMU.

####Usage  

a. Download and compile the AutoGUI  

	#git clone https://github.com/csudanthi/AutoGUI.git  
	#cd src  
	#make  

b. Connect to the Android system in QEMU  

	// connect AutoGUI with the vnc-server on port 5900, and then listen on 6000  
	#./autogui localhost 5900 6000  
	
	// connect the vnc-client with AutoGUI on port 6000  
	#vncviewer localhost:6000
	
c. Until now, you should see the desktop of Android which is forwared by AutoGUI. Then you can press "Ctrl + s" to start/stop the capture of VNC stream, and "Ctrl + i" to start/stop replay of VNC stream. You can read the code to get the full list of operation supportted in AutoGUI.   


## More details 
*** 

a. If you want to see more details, like message types. You can uncomment the declaration of ANALYZE_PKTS int main.h.  

b. For the newest version, AutoGUI supports both the 3.7 and 3.8 version of [RFB](http://en.wikipedia.org/wiki/RFB_protocol) protocal.  
To be as simplest as possible, AutoGUI only supports the RAW encoding for pixel data.  

c. At minimum, all that is necessary for the AutoGUI interceptor is to record the VNC session and dump out some formatted record files which are used by gem5 vncreplayer to replay. Though, for debug purpose, repalying with AutoGUI is also useful since the QEMU is faster than gem5.  

d. What will produce after recorded a vnc stream ?  
	UserIn.pkts: the data AutoGUI received from vnc-client  
	VncOut.pkts: the data AutoGUI received from vnc-server  
	fb.thres: the threshold for each frame  
	*.bmp: the bitmap data of captured frames  
	log.txt: log information  
