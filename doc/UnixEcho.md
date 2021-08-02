# The Unix and the Echo

The book *The Unix Programming Environment* makes a short
digression on some philosophical issues about the `echo`
command, one of these being whether `echo` should print
a blank line or nothing at all if it is given no arguments.
Current implementations of `echo` print a blank line, but
past versions didn't, and there were once great debates
on the subject. **Doug McIlroy** imparted the right
feelings of mysticism in his discussion of the topic:

> There dwelt in the land of New Jersey the UNIX, a fair maid
> whom savants traveled far to admire. Dazzled by her purity,
> all sought to expose her, one for her virginal grace, another
> for her polished civility, yet another for her agility in
> performing exacting tasks seldom accomplished even in much
> richer lands. So large of heart and accomodating of nature
> was she that the UNIX adopted all but the unsufferably rich
> of her suitors. Soon many offspring grew and prospered and
> spread to the ends of the earth.
>
> Nature herself smiled and answered to the UNIX more eagerly
> than to other mortal beings. Humbler folk, who knew little
> of more courtly manners, delighted in her echo, so precise
> and crystal clear they scarce believed she could be answered
> by the same rocks and woods that so garbled their own shouts
> into the wilderness. And the compliant UNIX obliged with
> perfect echoes of whatever she was asked.
>
> When one impatient swain asked the UNIX, ‘Echo nothing’, the
> UNIX obligingly opened her mouth, echoed nothing, and closed
> it again.
>
> ‘Whatever do you mean,’ the youth demanded, ‘opening your
> mouth like that? Henceforth never open your mouth when you
> are supposed to echo nothing!’ And the UNIX obliged.
>
> ‘But I want a perfect performance, even when you echo nothing,’
> pleaded a sensitive youth, ‘and no echoes can come from a closed
> mouth.’ Not wishing to offend either one, the UNIX agreed to say
> different nothings for the impatient youth and the sensitive
> youth. She called the sensitive nothing ‘`\n`.’
>
> Yet now when she said ‘`\n`,’ she was really not saying nothing
> so she had to open her mouth twice, once to say ‘`\n`,’ and once
> to say nothing, and so she did not please the sensitive youth,
> who said forthwith, ‘The `\n` sounds like a perfect nothing to me,
> but the second ruins it. I want you to take back one of them.’
> So the UNIX, who could not abide offending, agreed to undo
> some echoes and called that ‘`\c`’. Now the sensitive youth could
> hear a perfect echo of nothing by asking for ‘`\n`’ and ‘`\c`’
> together. But they say that he died of a surfeit of notation
> before he ever heard one.

Taken from *The Unix Programming Environment* by Brian W. Kernighan
and Rob Pike, Prentice-Hall, 1984.
