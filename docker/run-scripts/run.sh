#!/bin/sh
######################
# A quick and dirty shell script to run profanity with a user mounted config file.
#
# *NIX
#
# This is for *nix based systems of course. However Profanity deals with pathing
# may need to be recognized as a multi-option scenario here. 
#
# Example Windows Addition (which the author has no idea how to work with)
#  # WINDOWS USERS
#  # 
#  # Comment out "user_home" and the other "config_dir". Use this instead.
#  # Naturally where this lives largely depends on how profanity was installed 
#  # or works on Windows. 
#   config_dir='C:\%APPDATA%\profanity'
# 
######################

######################
  ### User Editable section
	# Instructions
	#   Modify variables with "##" above them. Leave the rest
	#   alone. 
	#
	## Edit user_home, to the appropriate user path.
  user_home='/home/example'
	# Leave this alone. 
  config_dir="$user_home/.config/profanity"
  ## Set the toolname you use to execute profanity images.
  virt_tool='docker'
  ## The name of the container to boot up (should you utilize multiple). 
  img_name='profanity.debian'
	### END User Editable section
######################


######################
# Meat and potatoes 
#
# Notes
#   In my testing themes are a problem in Docker containers. This appears to be
#   a limitation of terminal in these spaces.
#
#   Example 
#     Copied everforest theme to: 
#       $user_home/.config/profanity/themes/everforest
#
#       It did not work despite the config being set correctly. 
#
######################
#
# shutdown previous iteration first, and assume --rm is present in primary run.
$virt_tool stop $img_name
$virt_tool run -it --rm \
  -v $config_dir:/root/.config/profanity \
  $img_name


