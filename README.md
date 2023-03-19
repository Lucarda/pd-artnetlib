# pd-artnetlib

turn your Pd patch into an Art-Net controller.

Art-Net is an Ethernet protocol based on the TCP/IP protocol suite. Its purpose is to allow
transfer of large amounts of DMX512 data over a wide area using standard networking
technology.

https://en.wikipedia.org/wiki/Art-Net

https://www.artisticlicence.com/WebSiteMaster/User%20Guides/art-net.pdf 


----------------


**artnetlib** is a Pd library with 5 objects:

- [artnetfromarray]

  - polls a Pd array and convert the values to a list of DMX 1 byte ints



- [artnetsend]

  - format a Pd list of ints with an _ArtDMX_ header where you specify "physical" and "universe".



- [artnetudp]

  - send the _ArtDMX_ package to a specified ip

  - sends _ArtPoll_ and receive _ArtPollReply_ (used to discover the presence of other Controllers, Nodes and Media Servers.)

  - receive data from other Art-Net compatible devices 	



- [artnetroute]

  - routes received _ArtDMX_ packages according to its "physical" and "universe".



- [artnettoarray]

  convert _ArtDMX_ packages to a Pd list.

  
--------------------

repository: https://github.com/Lucarda/pd-artnetlib

bug reports: https://github.com/Lucarda/pd-artnetlib/issues
