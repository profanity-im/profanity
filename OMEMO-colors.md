# OMEMO colors - colorful OMEMO chats

The main goal of this MarkDown is to organize the ideas for this feature that has the potential to become an interesting and useful feature for every profanity's user, IMHO. The chosen color must not cause eyestrain on long OMEMO chats sessions. I'll try to keep this document updated as I make progress in the development.

## TODO list
- [ ] Understand how to implement the option `/omemo color on|off` (default: off).
- [ ] Implement `/omemo color on|off`.
- [ ] Understand how `/save` works.
- [ ] Implement sent OMEMO messages are white, received OMEMO messages are darker white.
- [ ] When `/omemo color on`, implement sent OMEMO messages are white, received OMEMO messages are darker white.
- [ ] Understand how to insert `color` in the autocompletion.

-------------------------------
- [ ] Support OMEMO colors on MUC.
- [ ] Develop a contrast checker to choose which color should be used based on the loaded theme.
- [ ] Make a working example of OMEMO messages. Sent OMEMO messages will be blue whereas received OMEMO messages will be of a darker blue. This choice should work well with the default theme of profanity without causing eyestrain. Perhaps, a better idea would be leave sent OMEMO messages white and make received OMEMO messages darker white, in this case it would be advised to set the `/omemo char <symbol>`.

Further ideas:
- [ ] (optional) create a background around OMEMO messages.
- [ ] opaqueness of non-OMEMO messages?

DISCLAIMER
This is an open source project. This feature is not about me, it is about us. Please, feel free to contribute by sharing ideas, developing code, commenting etc. :)