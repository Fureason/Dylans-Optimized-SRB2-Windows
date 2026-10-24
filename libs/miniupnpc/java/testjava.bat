@echo off
set JAVA=java
set JAVAC=javac

set CP=miniupnpc_win32.jar;.

%JAVAC% -cp "%CP%" JavaBridgeTest.java || exit 1
%JAVA% -cp "%CP%" JavaBridgeTest 12345 UDP || exit 1
