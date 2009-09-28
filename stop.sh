#!/bin/bash
echo "Shuting down the Streaming Server"
kill `pidof streamer`
