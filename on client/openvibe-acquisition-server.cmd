@echo off
call dependencies\set-env.cmd

cd bin
OpenViBE-acquisition-server-dynamic.exe

pause
