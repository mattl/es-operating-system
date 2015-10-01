# Requesting Code Review #

To merge your code into the trunk, please follow the following steps:

```
$ cd [YOUR WORKING COPY OF THE MAIN TRUNK]
$ svn update
```

Make your changes. Then create a copy of the main trunk in your personal branch and commit your change there:

```
$ svn copy https://es-operating-system.googlecode.com/svn/trunk https://es-operating-system.googlecode.com/svn/branches/gsoc2009/[YOUR DIRECTORY]
$ svn switch https://es-operating-system.googlecode.com/svn/branches/gsoc2009/[YOUR DIRECTORY]/trunk
$ svn commit
```

Note: Your local changes will remain when you svn switch your working copy to the branch (see [the manual](http://svnbook.red-bean.com/en/1.0/ch04s05.html)).

To request code review, please visit,

> http://code.google.com/p/es-operating-system/source/detail?r=[REVISION # OF THE CHANGE]

and click "Request Code Review" link. Fill in the form and select your mentor(s) in the Owner field. Please also include es-operating-system-dev@googlegroups.com in the Cc list. Then click "Submit issue". Please read [this page](http://code.google.com/p/support/wiki/CodeReviews) for further information about code review.

Once your mentor(s) give you positive score(s) for your change, merge your change into the main trunk:

```
$ svn switch https://es-operating-system.googlecode.com/svn/trunk
$ svn merge -r [BRANCHED OUT REVISION #]:[YOUR CHANGE REVISION #] https://es-operating-system.googlecode.com/svn/branches/gsoc2009/[YOUR DIRECTORY]/trunk
```

Now you can commit your change to the main trunk. Please commit it with a meaningful log message since it will be included in the ChangeLog file. Please read [this page](http://www.gnu.org/prep/standards/html_node/Change-Logs.html) for how to write a change logs:

```
$ svn commit
```

Once you see the status of your code review request is changed to a closed status like "Done", you can remove your trunk copy in your branch.

```
$ svn delete https://es-operating-system.googlecode.com/svn/branches/gsoc2009/[YOUR DIRECTORY]/trunk
```

**Note**: We will probably switch over to Reitveld sometime soon. cf. http://code.google.com/p/rietveld/