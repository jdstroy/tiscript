![http://www.terrainformatica.com/tiscript/logo.jpg](http://www.terrainformatica.com/tiscript/logo.jpg)

[TIScript](http://www.terrainformatica.com/wiki/tiscript:start) is a programming language and an implementation of a compiler, a virtual machine running bytecodes and a runtime.

TIScript uses well known, time-proven and popular language constructs of [ECMAScript (JavaScript)](http://en.wikipedia.org/wiki/ECMAScript).

Here is [Comparison of TIScript and JavaScript](http://www.codeproject.com/KB/recipes/TIScript.aspx)

Features:

  * TIScript supports JavaScript runtime behavior as closely as possible.
  * TIScript uses JavaScript notation and syntax in full but with some additions, e.g. [lambda functions](http://www.terrainformatica.com/wiki/doku.php?id=tiscript:functions), [types](http://www.terrainformatica.com/wiki/doku.php?id=tiscript:classes) (classes and namespaces) and properties.

  * TIScript uses a simplified prototype model so that
> > `'string'.prototype === String`
> > evaluates to `true` in TIScript

  * TIScript supports template content generation similar to PHP and ASP:
> > `some literal text <% script inclusion %> some more literal text`

  * TIScript has builtin and native persistence with the classes [Storage](http://tiscript.googlecode.com/svn/trunk/doc/Storage.htm) and [Index](http://tiscript.googlecode.com/svn/trunk/doc/Index.htm). We use the term JSON-DB as it is a database which stores JSON objects. We believe that TIScript is the first language to use a JSON-DB idiom.

SVN source distribution includes a makefile and a project file for Dev-C++ (Bloodshed Software.) If you have Dev-C++ and/or GCC/MinGW installed then you can compile it out of the box.

TIScript is used as a scripting engine of the [Sciter](http://www.terrainformatica.com/sciter/).

In **downloads** - tiscript.zip - command line interpretter, does not require installation.
In **source** - source of it.

[Concepts and source code structure](http://code.google.com/p/tiscript/wiki/TIScriptOverview).

[Language and Runtime](http://code.google.com/p/tiscript/wiki/LanguageAndRuntime)

Our gratitude:
  * to [David Betz](http://www.mv.com/ipusers/xlisper/) for his BOB language implementation. TIScript is derived from BOB in many aspects.
  * to [Konstantin Knizhnik](http://www.garret.ru/~knizhnik/) for his [DyBASE](http://www.garret.ru/~knizhnik/dybase.html) engine - core of the TIScript persistence.

&lt;wiki:gadget url="http://www.ohloh.net/p/48600/widgets/project\_basic\_stats.xml" height="220"  border="1" /&gt;

&lt;wiki:gadget url="http://www.ohloh.net/p/48600/widgets/project\_users.xml?style=gray" height="100"  border="0" /&gt;