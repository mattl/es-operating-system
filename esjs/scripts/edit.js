/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following books:
 *
 *   Copyright (C) 1976 by Bell Laboratories, Inc.
 *   Copyright (C) 1981 by Bell Laboratories, Inc., and Whitesmiths Ltd.
 *
 *   B. W. Kernighan and P. J. Plauger, Software Tools, Addison-Wesley, 1976.
 *   B. W. Kernighan and P. J. Plauger, Software Tools in Pascal, Addison-Wesley, 1981.
 *
 *   http://cm.bell-labs.com/cm/cs/who/bwk/toolsbook/index.html
 *
 *     You are welcome to make whatever use you can of them, subject to
 *     the fact that they are copyrighted by Bell Labs. If you do find a
 *     use, we would appreciate acknowledgement of the original source of
 *     the material.
 *
 *     No warranties, etc., etc. -- this is old code indeed.
 *
 *                                                   -- Brian Kernighan
 */

var buf;        // edit buffer

var line1;      // first line number
var line2;      // second line number
var nlines;     // # of line numbers specified
var curln;      // current line -- value of dot
var lastln;     // last line -- value of $

var term0 = /^[ \t]*/;                              // blanks
var term1 = /^(([0-9]+)|(\.)|(\$)|(\/((\\\/|[^\/])*)\/)|(\\((\\\\|[^\\])*)\\))/;  // line number component
var term2 = /^[+-][0-9]+/;                          // + or - term
var term3 = /(.)((\\\1|.)*?)\1((\\\1|.)*?)\1(g?)/;  // substitute pattern
var term4 = /^([ \t]+((\\ |[^ \t\n])+))?[ \t]*\n/;  // file name
var term5 = /[gx][ \t]*(.)((\\\1|.)*?)\1/;          // global pattern
var term6 = /^[^\n]*\n/;                            // single line

var pat = /(?:)/;   // pattern
var lin;            // input line
var savefile;       // remembered file name

var more;
var cursave;
var sub;

/** Prints a message and continue
 * @param   m  string to be printed
 */
function message(m)
{
    stdout.write(m + '\n', String(m).length + 1);
}

/** Gets mark from nth line.
 * @param   n  the line number
 */
function getmark(n)
{
    return buf[n].mark;
}

/** Puts mark m on nth line.
 * @param   n  the line number
 * @param   m  true or false
 */
function putmark(n, m)
{
    buf[n].mark = m;
}

/** Initializes for new file
 */
function clrbuf()
{
    ;   // nothing to do
}

/** Gets text from line n.
 * @param   n  the line number
 * @return  the text at line n
 */
function gettxt(n)
{
    return buf[n].txt;
}

/** Reverses buf[n1]...buf[n2].
 */
function reverse(n1, n2)
{
    while (n1 < n2)
    {
        var temp = buf[n1];
        buf[n1] = buf[n2];
        buf[n2] = temp;
        ++n1;
        --n2;
    }
}

/** Moves block of lines n1..n2 to after n3.
 */
function blkmove(n1, n2, n3)
{
    if (n3 < n1 - 1)
    {
        reverse(n3 + 1, n1 - 1);
        reverse(n1, n2);
        reverse(n3 + 1, n2);
    }
    else if (n2 < n3)
    {
        reverse(n1, n2);
        reverse(n2 + 1, n3);
        reverse(n1, n3);
    }
}

/** Initializes line storage buffer.
 */
function setbuf()
{
    buf = [ { txt: "", mark: false} ];
    curln = 0;
    lastln = 0;
}

/** Puts text from lin after curln
 */
function puttxt(lin)
{
    ++lastln;
    buf[lastln] = { txt: lin, mark: false};
    blkmove(lastln, lastln, curln);
    ++curln;
}

/** Gets a line from stdin.
 * @return the string of line read
 */
function gets()
{
    lin = "";
    var c;
    while ((c = stdin.read(1)) != '')
    {
        if (c == '\r' || c == '\n')
        {
            lin += '\n';
            stdout.write("\n", 1);
            return;
        }
        if (c == '\b' || c == '\x7f')
        {
            if (0 < lin.length)
            {
                stdout.write("\b \b", 3);
                lin = lin.slice(0, -1);
            }
            continue;
        }
        stdout.write(c, 1);
        lin += c;
    }
    throw "EOF";
}

/** Gets line number after n.
 * @return the line number after n.
 */
function nextln(n)
{
    return (lastln <= n) ? 0 : n + 1;
}

/** Gets line number before n.
 * @return the line number before n.
 */
function prevln(n)
{
    return (n <= 0) ? lastln : n - 1;
}

/** Finds next occurrence of pattern after line n.
 * @param   way  '/' to scan forward or '\' to scan backward
 * @return  the line number, or -1 on failure.
 */
function patscan(way)
{
    var n = curln;
    do
    {
        n = (way == '/') ? nextln(n) : prevln(n);
        line = gettxt(n);
        if (pat.test(line))
        {
            return n;
        }
    } while (n != curln);
    return -1;
}

/** Skips blanks and tabs at the head of s.
 * @param   s  the string to skip blanks,
 * @return  a string not begin with blanks
 */
function skipbl(s)
{
    var bl = term0.exec(s);
    if (bl)
    {
        s = s.substring(bl[0].length);
    }
    return s;
}

/** Gets one line number expression.
 * @return the line number, or a negative number on failure
 */
function getone()
{
    lin = skipbl(lin);
    var one = term1.exec(lin);  // 1st term
    var num;
    if (one)
    {
        lin = lin.substring(one[0].length);
        switch (one[1].charAt(0))
        {
        case '.':
            num = curln;
            break;
        case '$':
            num = lastln;
            break;
        case '/':
            if (one[6])
            {
                pat = new RegExp(one[6]);
            }
            num = patscan('/', pat);
            break;
        case '\\':
            if (one[9])
            {
                pat = new RegExp(one[9]);
            }
            num = patscan('/', pat);
            break;
        default:
            num = parseInt(one[1]);
            break;
        }
        // + or - term
        lin = skipbl(lin);
        var offset = term2.exec(lin);
        if (offset)
        {
            num += parseInt(offset[0]);
            lin = lin.substring(offset[0].length);
        }
        if (lastln < num)
        {
            return -1;
        }
        return num;
    }
    return -1;
}

/** Sets defaulted line numbers.
 * @param   def1  the default value for line1
 * @param   def2  the default value for line2
 * @return true on success, or false on failure.
 */
function setdefault(def1, def2)
{
    if (nlines == 0)
    {
        line1 = def1;
        line2 = def2;
    }
    if (line2 < line1 || line1 <= 0)
    {
        return false;
    }
    return true;
}

/** Gets list of line nums at lin.
 */
function getlist()
{
    line2 = 0;
    nlines = 0;
    var num;
repeat:
    while (0 <= (num = getone()))
    {
        line1 = line2;
        line2 = num;
        ++nlines;
        switch (lin.charAt(0))
        {
        case ';':
            curln = num;
            // FALL THROUGH
        case ',':
            lin = lin.substring(1);
            break;
        default:
            break repeat;
        }
    }
    nlines = Math.min(nlines, 2);
    if (nlines == 0)
    {
        line2 = curln;
    }
    if (nlines <= 1)
    {
        line1 = line2;
    }
}

/** Prints lines n1 through n2.
 * @param   n1  the first line number in the range
 * @param   n2  the last line number in the range
 * @return true on success, or false on failure.
 */
function doprint(n1, n2)
{
    if (n1 <= 0)
    {
        return false;
    }
    for (var i = n1; i <= n2; ++i)
    {
        var line = gettxt(i);
        stdout.write(line, line.length);
    }
    curln = n2;
    return true;
}

/** Appends lines after "line".
 * @param   line  the line number after which append lines.
 * @param   glob  true indicates under control of a global prefix. Otherwise, false.
 * @return true on success, or false on failure.
 */
function append(line, glob)
{
    if (glob)
    {
        return false;
    }

    curln = line;
    for (;;)
    {
        gets();
        if (lin == ".\n")
        {
            break;
        }
        puttxt(lin);
    }
    return true;
}

/** Deletes lines n1 through n2.
 * @return true on success, or false on failure.
 */
function deleteln(n1, n2)
{
    if (n1 <= 0)
    {
        return false;
    }
    blkmove(n1, n2, lastln);
    lastln -= (n2 - n1 + 1);
    curln = prevln(n1);
    buf.length = lastln + 1;
    return true;
}

/** Moves line1 through line2 after line3.
 * @return true on success, or false on failure.
 */
function move(line3)
{
    if (line1 <= 0 || (line1 <= line3 && line3 < line2))
    {
        return false;
    }

    blkmove(line1, line2, line3);
    if (line1 < line3)
    {
        curln = line3;
    }
    else
    {
        curln = line3 + (line2 - line1 + 1);
    }
    return true;
}

/** Gets right hand side of "s" command.
 * @return true on success, or false on failure.
 */
function getrhs()
{
    var one = term3.exec(lin);  // 2, 4, 6
    if (!one)
    {
        return false;
    }
    lin = lin.substring(one[0].length);
    pat = new RegExp(one[2], one[6]);
    sub = one[4];
    return true;
}

/** Substitutes "sub" for occurrences of pattern.
 * @param   glob  true indicates under control of a global prefix. Otherwise, false.
 * @return true on success, or false on failure.
 */
function subst(glob)
{
    var stat = glob ? true : false;
    for (var line = line1; line <= line2; ++line)
    {
        var from = gettxt(line);
        var to = from.replace(pat, sub);
        if (from != to)
        {
            deleteln(line, line);
            if (line == 1)  // Fix STiP
            {
                curln = 0;
            }
            puttxt(to);
            stat = true;
        }
    }
    return stat;
}

/** Checks for "p" after command
 * @return  true if the trailing "p" option is found. Otherwise, false.
 */
function ckp()
{
    lin = skipbl(lin);
    if (lin.charAt(0) == 'p')
    {
        lin = lin.substring(1);
        return true;
    }
    return false;
}

/** Gets file name from lin.
 */
function getfn()
{
    var t = term4.exec(lin);
    var fil = "";
    if (t)
    {
        if (t[2])
        {
            fil = t[2];
            if (!savefile)
            {
                savefile = fil;
            }
        }
        else if (savefile)
        {
            fil = savefile;
        }
    }
    return fil;
}

/** Reads "fil" after line n.
 * @param   fil  the file name
 * @return true on success, or false on failure.
 */
function doread(n, fil)
{
    var linebuf = '';

    /** Gets a line of text from a stream
     * @return the string of line read; "" implies end of file.
     */
    function getline(stream)
    {
        var inline = "";
        var a;

        while (!(a = term6.exec(linebuf)))
        {
            var c = stream.read(128);
            if (c == '')
            {
                if (linebuf == '')
                {
                    return '';
                }
                c = linebuf + '\n';
                linebuf = '';
                return c;
            }
            linebuf += c;
        }
        linebuf = linebuf.substring(a[0].length);
        return a[0];
    }

    try
    {
        var file = File(cwd.lookup(fil));
        var stream = file.stream;
        var count = 0;
        curln = n;
        var inline;
        while (inline = getline(stream))
        {
            puttxt(inline);
            ++count;
        }
        stream.release();
        stdout.write(count + '\n', String(count).length + 1);
    }
    catch (e)
    {
        return false;
    }
    return true;
}

/** Writes lines n1..n2 into file
 * @param   fil  the file name
 * @return true on success, or false on failure.
 */
function dowrite(n1, n2, fil)
{
    try
    {
        var unknown = cwd.bind(fil);
        if (!unknown)
        {
            unknown = cwd.lookup(fil);
        }
        var file = File(unknown);
        var stream = file.stream;
        stream.setSize(0);  // truncate
        for (var i = n1; i <= n2; ++i)
        {
            line = gettxt(i);
            stream.write(line, line.length);
        }
        stream.release();
        var count = n2 - n1 + 1;
        stdout.write(count + '\n', String(count).length + 1);
    }
    catch (e)
    {
        return false;
    }
    return true;
}

/** Handles all commands except globals
 * @param   glob  true indicates under control of a global prefix. Otherwise, false.
 * @return true on success, or false on failure.
 */
function docmd(glob)
{
    var pflag = false;      // may be set by d, m, s
    var status = false;
    var fil;

    var cmd = lin.charAt(0);
    lin = lin.substring(cmd.length);
    switch (cmd)
    {
    case 'p':
        if (status = setdefault(curln, curln))
        {
            status = doprint(line1, line2);
        }
        break;
    case '\n':
        if (nlines = 0)
        {
            line2 = nextln(curln);
        }
        status = doprint(line2, line2);
        break;
    case 'q':
        if (nlines == 0 && !glob)
        {
            more = false;
            status = true;
        }
        break;
    case 'a':
        status = append(line2, glob);
        break;
    case 'c':
        if (status = setdefault(curln, curln))
        {
            if (status = deleteln(line1, line2))
            {
                status = append(prevln(line1), glob);
            }
        }
        break;
    case 'd':
        pflag = ckp();
        if (status = setdefault(curln, curln))
        {
            if (status = deleteln(line1, line2))
            {
                if (nextln(curln) != 0)
                {
                    curln = nextln(curln);
                }
            }
        }
        break;
    case 'i':
        if (line2 == 0)
        {
            status = append(0, glob);
        }
        else
        {
            status = append(prevln(line2), glob);
        }
        break;
    case '=':
        pflag = ckp();
        stdout.write(line2 + '\n', String(line2).length + 1);
        status = true;
        break;
    case 'm':
        var line3 = getone();
        if (0 <= line3)
        {
            pflag = ckp();
            if (status = setdefault(curln, curln))
            {
                status = move(line3);
            }
        }
        break;
    case 's':
        if (getrhs())
        {
            pflag = ckp();
            if (status = setdefault(curln, curln))
            {
                status = subst(glob);
            }
        }
        break;
    case 'e':
        if (nlines == 0)
        {
            fil = getfn();
            if (fil)
            {
                savefile = fil;
                clrbuf();
                setbuf();
                status = doread(0, fil);
            }
        }
        break;
    case 'f':
        if (nlines == 0)
        {
            fil = getfn();
            if (fil)
            {
                savefile = fil;
                stdout.write(savefile + '\n', savefile.length + 1);
                status = true;
            }
        }
        break;
    case 'r':
        if (fil = getfn())
        {
            status = doread(line2, fil);
        }
        break;
    case 'w':
        if (fil = getfn())
        {
            if (status = setdefault(1, lastln))
            {
                status = dowrite(line1, line2, fil);
            }
        }
        break;
    default:
        break;
    }

    if (status && pflag)
    {
        doprint(curln, curln);
    }

    return status;
}

/** If global prefix, marks lines to be affected.
 * @return true on success, or false on failure.
 */
function ckglob()
{
    var one = term5.exec(lin);  // 2
    if (!one)
    {
        return false;
    }

    var gflag = (lin.charAt(0) == 'g') ? true : false;
    lin = lin.substring(one[0].length);
    pat = new RegExp(one[2]);

    if (setdefault(1, lastln))
    {
        for (var n = line1; n <= line2; ++n)
        {
            var temp = gettxt(n);
            putmark(n, pat.test(temp) ? gflag : !gflag);
        }
        for (var n = 1; n <= line1 - 1; ++n)
        {
            putmark(n, false);
        }
        for (var n = line2 + 1; n <= lastln; ++n)
        {
            putmark(n, false);
        }
        return true;
    }

    return false;
}

/** Issues command at lin on all marked lines.
 * @return true on success, or false on failure.
 */
function doglob()
{
    var status = true;
    var count = 0;
    var n = line1;
    var linsave = lin;
    do {
        if (getmark(n))
        {
            putmark(n, false);
            curln = n;
            cursave = curln;
            lin = linsave;
            getlist();
            if (status = docmd(true))
            {
                count = 0;
            }
        }
        else
        {
            n = nextln(n);
            ++count;
        }
    } while (status && count <= lastln);
    return status;
}

// main routine for text editor
setbuf();
savefile = "";
more = true;
try
{
    savefile = params[1];
    if (!doread(0, savefile))
    {
        message('?');
    }
}
catch (e)
{
}
while (more)
{
    cursave = curln;
    gets();
    getlist();
    if (ckglob())
    {
        status = doglob();
    }
    else
    {
        status = docmd(false);
    }

    if (!status)
    {
        message('?');
        curln = Math.min(cursave, lastln);
    }
}
