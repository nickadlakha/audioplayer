# audioplayer
Audioplayer based on ffmpeg libraries.

## Pre-requisites ##
* `sudo apt-get install -y libasound2-dev libavformat-dev libavcodec-dev libavutil-dev`

## Compiling ##
* `./configure --prefix=$HOME`
* `make`
* `make install`

## Running ##
* `~/bin/audioplayer file:///home/xyx/Music/xyz.mp3`
* `~/bin/audioplayer /home/xyx/Music/xyz.aac`
* `~/bin/audioplayer file:///home/xyx/Videos/xyz.mp4`
* `~/bin/audioplayer https://strm112.1.fm/loveclassics_mobile_mp3`
* `cat /home/xyx/Videos/xyz.mp4|~/bin/audioplayer -`
  
