NppSalt
=======

NppSalt is a simple Notepad++ Plugin. Currently it does two things:

1) It flashes the Notepad++ screen when it detects any of the open files has been changed by other programs;

2) When a file is about to be saved, if NppSalt detects that the file has been changed by other programs,
   it will copy the current changes to a "new" window and reload the file from disk. This prevents
   people from overwritting each other's changes. Note there seems no way for a plug-in to cancel out a 
   Notepad++ file save operation, hence this is the best NppSalt can do for now.
   
To compile the Plugin, simply open NppSalt.sln using Visual Studio 2010 (or high) and build.

To deploy, simply copy NppSalt.dll to Notepad++'s Plugins folder.


Peter Gu
pete.x.gu@gmail.com



