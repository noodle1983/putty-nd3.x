Comment
============
I notice that some antivirus software reports risk of virus in putty-nd, and I do look into this.
It turns out that the listening to the keyboard feature when the term win is active and the ways to handle the w32sock api in putty will trigger that report.
The way to solve this is to apply a code sign certification which can identify the software is released by me.
But it can't prevent me to do evil things, which I won't, and I have to pay for it :-(. 
I am good at software design, but not good at business, no plan to make money from this. So I decide to leave as it is. 

From putty-nd2, putty.exe is released alone when other applications in putty suite is not changed.
Please download putty_suite_without_putty_exe.zip to make a complete putty suite.

Any comment is appreciated.
Email: noodle1983@126.com 

And many thanks for your supporting and your coffee:)

[PayPal:https://www.paypal.me/noodle1983](https://www.paypal.me/noodle1983)

Or Alipay:![avatar][donation_ali]
Or Wechat:![avatar][donation_wx]


Old Version Upgrade Alerts
============

The old config data **prior to putty-nd6.0**  will be upgraded , so please backup the settings to files or Google Driver incase bad thing happens.


RELEASE NOTE
============

--------------------------------
putty-nd6.3
--------------------------------
1. Bugs	
	1. When template settings is applied, the password field in autocmd can be shown with encryped string. (The logic should be that, if it is show, it is not encryped. And it is hidden, it is encryped.)

2. Other changes
	1. cpu usage optimization: not to do the paint logic for the tabs not shown.
	2. full flow control on the receive msg, and 20% flow control on zmodem, so the unwilling output process can be killed quickly.
	3. show ssh banner by default.
	4. open sub-sessions in a new window and hold CTRL key to open in the same window.
	5. right click to show tab menu.
	6. update link for donation.
	
	
3. Ongoing 
	1. better ui to manage session on the google drive.


--------------------------------
putty-nd6.2
--------------------------------
1. Bugs	

2. Other changes
	1. re-enabled copying as rtf data.
	2. cmdline password is only valid for the first time.
	3. Ctrl+Shift+6 to hide the toolbar.
	4. Ctrl+Shift+K to close the active tab.
	
3. Ongoing 
	1. better ui to manage session on the google drive.

--------------------------------
putty-nd6.1 
--------------------------------
1. Bugs	
	1. Crash when autocmd prompt is username, and input is username.
	2. Font can't be saved.
	3. Int config sometimes can't be saved.

2. Other changes
	1. Make new session button act the same with new session in popup menu.
	2. Use group name as prefix when new session.
	3. Color autocmd gray if it is not applied.
	4. Remove adb from the release package. Instead it is recommanded to start ssh server in git, then login local server to exec any windows cmd.
	
3. Ongoing 

--------------------------------
putty-nd6.0 THE OLD CONFIG DATA WILL BE UPGRADED WHEN IT STARTS, SO BACKUP THE OLD DATA TO FILES OR GOODLE IN CASE BAD THINGS HAPPEN.
--------------------------------
1. Bugs	

2. Other changes
	1. Default Setting as template for all sessions, folder setting as template for subsessions.
	2. Support Sub-group, max 250 str for the session path.
	
3. Ongoing 
	
--------------------------------
putty-nd5.4
--------------------------------
1. Bugs	

2. Other changes
	1. Make search result valid after new line is added.
	2. Put input box and messagebox to topmost.
	3. Disable auto scrolling to end when click search or mouse wheel up, and re-enabled when send any key or scroll to end again.
	4. Make different exe path lead to different running processes.
	5. Support set tab name from cmd like echo -ne "\e]2;tab_name\a".
	6. Set the max length of each autocmd to 4000.
	7. Enlarge the listbox of autocmd.
	8. Clear screen if clear scrollback in logging.
	
3. Ongoing 
	1. Default Setting as template, folder setting as template

--------------------------------
putty-nd5.3
--------------------------------
1. Bugs	
    1. Having multiple inactive sessions, sharing input (pressing enter to restart the sessions) is not possible.
    2. When changing a character in a group name from upper to lower case, sessions are lost.

2. Other changes
	1. Max 20 sessions are opened simultaneously with "Open Subsessions"
	2. F2 to rename session/tab title.
	3. Add session template to start sshserver in git installed media.
	
--------------------------------
putty-nd5.2
--------------------------------
1. Bugs	
    1. not to update icon when disconnected.
    2. not to show message box when fatal error happens, print to term instead.
	
--------------------------------
putty-nd5.1
--------------------------------
1. Features	
    1. Ctrl+Shift+E to edit the title on tab temporarily.
    2. Support more automate commands, 600 max.
    3. Make the group open-able.
    4. Confirmation check before close a tab, can be configed via Window -> Behaviour.
	5. Configurable for scrolling to the end via toolbar.
	
2. Bugs
 	
3. Other changes
	
4. Known issue

--------------------------------
putty-nd5.0
--------------------------------
1. Features	
    1. backup/restore sessions to/from Google.
	
2. Bugs
 	
3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.5
--------------------------------
1. Features	
    1. make the pipe command configurable in "Connection/ADB", which can be used in others exe program besides adb.exe.
	
2. Bugs
    1. high cpu usage, many thanks to Alexey(alexey3411@rambler.ru). A new thread is involed in version4.4 which works as https://github.com/noodle1983/framework-nd/blob/master/Processor/src/BoostWorker.cpp#L217. As read in the line, 500 microseconds is too small and results in high cpu usage.
	
3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.4
--------------------------------
1. Features	
	
2. Bugs
	1. can't filter the right sessions, empty session instead.
	
3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.3
--------------------------------
1. Features
	1. pipe to adb.exe abstracted from android sdk
	
2. Bugs
	1. crash when hostname can't be resolved
	2. sharing input does not work.
3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.2
--------------------------------
1. Features
2. Bugs
	1. crash when import session.
	2. crash when reconfig.
	3. crash when sshkey need password.
	4. make it work for xp sp3.

3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.1 debug
--------------------------------
1. Features
	1. Close window on exit.

2. Bugs
	1. type "exit" would crash.

3. Other changes
	
4. Known issue

--------------------------------
putty-nd4.0 debug
--------------------------------
1. Features
	1. add index on tab.
	2. Sync with the PuTTY mainline.
	3. ... (Sth i can't remember, whatever)

2. Bugs
	It should be many bugs, please report.

3. Other changes
	
4. Known issue

--------------------------------
putty-nd3.1
--------------------------------
1. Features
	1. auto load SecureCrt sessions.
	2. add new session button to session dialog.

2. Bugs

3. Other changes
	1. shortcut key adjust: Ctrl + ` to select next tab; Ctrl + tab to select previous tab.
	
4. Known issue

--------------------------------
putty-nd3.0
--------------------------------
1. Features
	1. zmodem support(test only for sz/rz, including bin format, but no resuming download).
	2. share keyboard input among tabs.
	3. enlarge setting dialog and extract the right click menu items to buttons.
	4. openssh key support(auto pri-key conversion).
	5. act as pageant if it is not running.

2. Bugs
	1. refresh issue: other window's item will show in putty-nd.
	2. window activation issue: it would activate wrong window after a new tab is created.

3. Other changes
	1. shortcut key adjust: Ctrl + ` to select next tab; Ctrl + tab to select previous tab.
	
4. Known issue

--------------------------------
putty-nd2.6
--------------------------------
1. Features

2. Bugs
	1. delete chinese close tooltips.
	2. resize the tab according the to the length of the titile, and hide left if no enough room for it.
	3. auto cmd failed to be applied sometimes, log the error to event log.
	4. change default font size to 14, and enable x11 forwarding by default.
	5. crash bug when the session is ended by remote server.

3. Other changes
	
4. Known issue

--------------------------------
putty-nd2.5
--------------------------------
1. Features

2. Bugs
	1. Crashes on unresolvable server address.
	2. Doesn't work with serial connections.
	3. Cmd line doesn't work for the first window.

3. Other changes
	
4. Known issue

--------------------------------
putty_nd2.4
--------------------------------
1. Features
	1. Ignore case for sessions search.
	2. Fill default setting with default x11 display string.

2. Bugs
	1. Refresh issue for the main window.(rollback)
	2. When cancel button in session dialog is clicked, do not bring the main window to front.
	3. Cursor would disappear from time to time and never show up.
	4. The group expanding state can't be saved.
	5. It can't be scrolled smoothly with mouse dragging.

3. Other changes
	1. refractor in main window handling.
	
4. Known issue

--------------------------------
putty_nd2.3
--------------------------------
1. Features

2. Bugs
	1. It is impossible to enter key after connection failed.
	2. crash issue when hostname is unknow or backend failed to start.
	3. Term is resized very narrow when minimized. 

3. Other changes

4. Known issue
	1. It can't be scrolled smoothly with mouse dragging.

--------------------------------
putty_nd2.2
--------------------------------
1. Features

2. Bugs
	1. refresh issue for the main window.
	2. settings will save to previous session when the name is changed and open the session.
	3. the icon issue for big icon on win7/8.

3. Other changes
	1. set default charset to utf-8.
	2. show config setting in the middle of the screen.

--------------------------------
putty_nd2.1
--------------------------------
1. Features

2. Bugs
	1. crash issue when session is closed with vim or something open, or portforward is applied.
	2. not possible to enter the non-ASCII char.

3. Other changes
	1. set the default protocol to ssh in session config dialog.
	2. set the default focus to searchbar in session config dialog.
	3. disable the Alt/Ctrl + Arrow shortcut, because it is occupied by emacs.
	4. change the new session shortcut to Ctrl+Shift+T, which is from the Linux console. And dup session shortcut key to Ctrl+Shift+N.

4. Known issue

--------------------------------
putty_nd2.0
--------------------------------
1. Features
	1. The REAL chrome style UI.
	2. Draggable tabs among putty windows.

2. Bugs

3. Other changes

4. Known issue


--------------------------------
putty6.0_nd1.17
--------------------------------
1. Bugs
    1. search bar issue in config dialog in 1.16: 
    the 1st session data will be clean after being filtered. 
    hopes it didn't ruin your data.

2. Other changes

3. Known issue
    1. crashes happens when maximized and changing the font. The root cause is unknown yet.

--------------------------------
putty6.0_nd1.16
--------------------------------
1. Bugs

2. Other changes
    1. Add a serach bar for the session tree view in the config dialog 

3. Known issue
    1. crashes happens when maximized and changing the font. The root cause is unknown yet.

--------------------------------
putty6.0_nd1.15
--------------------------------
1. Bugs
    1. The window is maximized with wrong size on my second screen on Windows 7.

2. Other changes
    1. The setting dialog and the main window are shown in the same screen.
    2. Type down arrow key to sellect the session in the setting dialog.

3. Known issue
    1. crashes happens when maximized and changing the font. The root cause is unknown yet.

--------------------------------
putty6.0_nd1.14
--------------------------------
1. Bugs
    1. The window is maximized with wrong size on my second screen on Windows 7.
    2. Crash bug when telnet/raw tab exits from time to time.

2. Other changes
    1. expand the width of the session treeview.

--------------------------------
putty6.0_nd1.13
--------------------------------
1. Bugs
    1. config dialog can be opened twice and then crash.

2. Other changes
    1. backup/restore(import/export) the sessions.
    2. make a config item valid: Session->Close window/tab on exit.

--------------------------------
putty6.0_nd1.12
--------------------------------
1. Bugs
    1. crash bug: ldisc's frontend should be set as tabitem. it happens when connecting to a raw telnet server.
    2. one Makefile issue while cross-compiling putty-nd on linux for Windows in Recipe file.

2. Other changes
    1. keyboard shortcuts: Ctrl+[Shift]+Tab to shift tab; Ctrl+[Shift]+` to move tab; Ctrl+Shift+n to new a session.

--------------------------------
putty6.0_nd1.11
--------------------------------
1. Other changes
    1. make it possible to change the tab title dynamically. How to do:
       a. un-tick setting Terminal->Featrues->Disable remote-controlled tab title...
       b. run cmd:PROMPT_COMMAND='echo -ne "\033]0;the_tab_title\007"'. The tab title can contain the system env, for example:
           PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOST}: ${PWD}\007"'
    2. a very good idea from Lokesh: One click logging feature to capture terminal prints. This modified code will have an option for capturing the onscreen prints to a log file. Even though this is already a feature of Putty in "logging" section but there is no straight forward mechanism to Start & Stop whenever required. So two options have been provided:
       a. A menu entry "Start Logging" when we right click anywhere inside terminal (for this to work "Selection" behaviour has to be "windows").
       b. A toolbar button ">" for Start & Stop Logging.
    3. more options for log file's name(&S for session name, and &P for the desktop path).


--------------------------------
putty6.0_nd1.10
--------------------------------
1. Other changes
    1. keyboard shortcuts for tabs: Alt+Num: switch to tab; Alt+`/Right: iterate the tab forward; Alt+Left; iterate the tab backword; Ctrl+Shift+t: duplicate the tab.

--------------------------------
putty6.0_nd1.9
--------------------------------
1. Other changes
    1. when duplicating a tab, the new tab will be placed next to the current tab.

--------------------------------
putty6.0_nd1.8
--------------------------------
1. Bug fixed
    1. when using Vim in a session for a long time and then creating another session, it may crash. It happened in my work env from time to time. Root cause is unknown. I changed something by guessing. And the issue didn't happen for 2 weeks.

2. Other changes
    1. When openning a session via Default Setting, the session will be saved with name "tmp/host:port". And the title will show as "host:port" instead of "Default Setting".

--------------------------------
putty6.0_nd1.7
--------------------------------
1. Bug fixed
    1. after reconfiguration, current selected session in tab will be saved to the lastest open session.

2. Other changes
    1. draw the main window's edge

--------------------------------
putty6.0_nd1.6
--------------------------------
1. Bug fixed
    1. crash at exit when one of the session's hostname can't be resolved. the 
crash only happens when the process exits.

2. Other changes
    1. when enter key is typed on a closed session, it restarts the session.
    2. tab title shows gray if session is closed.

--------------------------------
putty6.0_nd1.5
--------------------------------
1. New Features
    1. searchbar

2. Other changes
    1. Add tooltips for the toolbar buttons.
    2. In the session manager, save the expanding status of the session group.

--------------------------------
putty6.0_nd1.4
--------------------------------
1. Bugs fixed
    1.1. do not reset win_title after re-configured. 
    1.2. resize the length of the tabbar, to avoid covered by the system botton.
	
--------------------------------
putty6.0_nd1.3
--------------------------------
1. Bugs fixed
    1.1. assert failed when reconfig. 
    1.2. fullscreen is not supported yet, fix a scrollbar issue when zoomed.
	
--------------------------------
putty6.0_nd1.2
--------------------------------
1. Bugs fixed
    1.1. LICENSE. 
    1.2. resize term when swith tab.

--------------------------------
putty6.0_nd1.1
--------------------------------
1. Bugs fixed
    1.1. crash when paste by shift+insert. 
    1.2. not to show special menu when right click on the page.
    1.3. fix bug when maximize on multi-monitor.
    1.4. bind logevent to a tab; if not appliable, dump to debug log.

--------------------------------
putty6.0_nd1.0
--------------------------------
1. New Features
    1. tabbar

2. Other changes
    2.1 merge from putty6.0_nd0.3

3. Bugs fixed
    3.1. window's title is not set. 
    3.2. It only has a left-top archor when sizing.
    3.3. no right click memu

--------------------------------
putty6.0_nd0.3
-------------------------------- 
1. Bugs Fix
    1.1 rename the last session and do some changes and open, the cfg is saved as previous session.

--------------------------------
putty6.0_nd1.0-pre
--------------------------------
1. New Features
    1. tabbar

2. Other changes
    2.1 merge from putty6.0_nd0.2

3. Bugs remains
    3.1. window's title is not set. 
    3.2. It only has a left-top archor when sizing.
    3.3. no right click memu

--------------------------------
putty6.0_nd0.2
-------------------------------- 
1. Bugs Fix
    2.1 automate logon does not work sometimes.
    

--------------------------------
putty6.0_nd0.1
--------------------------------
1. New Features
    1.1 Session Management.
    1.2 Automate Logon.
    1.3 Scroll lines can be configured when the middle button of the mouse is scrolled. The default lines' number is 3.

[donation_ali]:data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGwAAABsCAMAAAC4uKf/AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAMAUExURQAAAAEBAgICAgUFBQkJCQwLCwwMDBAQEBISEhYWFhgXFxgYGB0dHSIiIiUlJSgoKCoqKSoqKi4uLjEwMTU1NTg4ODo6Oj09PUBAQEJCQkVFRUhISE1NTVBQT1FRUVRTVFRUVFlZWVxbW1xcW11dXWBgYGJiYmVlZWlpaW1tbXJycnV1dXd4eHl4dXh4eHp6ent7fH19fYV5bouAdI6Cc46CdZCDdZGFdZCDeJaKdpWJeZeMeJmNepmOfZyPfJuQfJ2Qe52RfqCTfaCUf4KCgoOEhIWFhYmJiY2NjZqPgJ2PgJuSgZ2SgZ2Sh52Vg5+WhZCQkJaWlpqYlpqampubnJycnKGVgaGVhaWXhaCXiKaZhqOZiqaai6ScjqiaiKqdiamdjKyei6yejKegj66ijaaglqmhkqmhla2hkKyjlKihmaignaukmqqkna2jmK+mma2mnbChjbGkj7KlkbKmlbWnlbWok7WplbKpmbCqnrSsnbisl7mtmbqvnKGhoaSkpK2moK+ooKmpqa2trbGqorOsprWtobeuprWuqLmvoLawpLOwq7ewqbuxoLmxp72yoryypb20o760pbmyqrqzrbu0qrq0rL21qb22rbCwsLKysrSysrW0s7W1tbm1sL22sL+5sri4uL+9ur29vcC1psC3qMG6rsS6qcS7r8W8q8S8rcG6scG7tcK8scK9tcS9ssW+tMG9uMC/vcW+uMbAtcXCvsnBtsnCucjDvsvEu8rEvMzFu83Gvc/IvsHBwcTExMrFwMjGxczGwM/Jw87JxMjIyMrKys3NzdHKwdLLxdLMxdTNxdLNydXOyNXQytbRzNjRytjSzNnVztzXz9HR0dTU1NbW1tnV0N3X0NjY1tzY0d3a1djY2Nra2tzc3N7e3uDc0+Hd1uDd2eLf3ODg4OLi4uXk4eTk5Obm5ujo6Orq6uzs6uzs7O7u7vDw8PPz8fLy8vTz8/T09Pb29vn59/j4+Pr5+vr6+vz6+/39+fz9+/z8/P39/v3+/f7+/gAAAJEetU8AAAEAdFJOU////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wBT9wclAAAACXBIWXMAAA7DAAAOwwHHb6hkAAAAGXRFWHRTb2Z0d2FyZQBwYWludC5uZXQgNC4wLjE2RGmv9QAADrpJREFUaEPtWnt821UVvyYhTWweps3aLY0x9ROWPpRQX+jc0GFsi6Kkhg0dLx8ovnlsCiIETHn4QBEQWIYBYxGRCqsTNtSRIh+y4nCd1Kab0KyKnU1LF2NiDPKDq+ece/No1qYd48M/7Pv5rEl+r+/v3HvuOd9z7tiLyouvDl5Q/sdeeom/Ovgvkr06VMDy0jGyV2Ksjw3jKzGKrx1vnBwbn4sJztPjCYVPjo/neRLPzfCsvGRSURLjKa5M4O8UPzR+gPNMxf3jk2UTUOGNy1gFzAoPMV1aWc5Uk3wlnvTwmLymQcnqWZAr9fi7l/cwo8IjlQ9YduRkfF4yfrRk6iZ7CXUMLIvZncGQ2+6Y4T67XcecIb/dXs/Udrs71Ou0d4d6jcxit/tCLcKyOQ+wsKqWaWfL7O5GMs5ntaxPHFUaYZisCr+K1eb4INNMcyeOWxfnDvggMu2hsgd0VSfTzPLULCHFuQ/JcrMHNKwXj2S50qTTCDJ9cjbM1OOzSIJkLp1OV09kKZ4XD5jN8e5FLXNqCHZJFtZoGKMjAc5zmQ5BBkfU+EclyXKZTCbLhWXjWvGEPr64ZXbhUU2SLFR0sHU4QF5JVu52YJkEWTZGb8BYZAmWVZD11dfJm0tk4ToBNTPWwwedmBmKyzkrkR2xZfAYrXjTEpm0g1y/ALnOjs4yJZesURP8OQANIwC/p2rVRJbP5RTuV5uO3rKIwTI+SWgxALSCbNoI342jkxkk6zQYwjw9mSQHOao5w3AlhorCFa0zzpPgi7jOCO0YrgjkjUcyZ30BAqxjWmcHA+uOd8UlmSfgYDoXwB0IdDCVE7+6XN5AoA0+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
[donation_wx]:data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGUAAABkCAMAAACo/g5YAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAMAUExURQAAAAICAgUFBQgICAoKCgwMDA4ODg8PEBAQEBISEhQUFBYWFhgYGBoZGhoaGhwcHB4eHiAgICIiIiQkJCYmJigoKCoqKiwsLC4uLjAwMDIyMjY2Njg3Nzk5OT09PUBAP0FBQUREREZGRklJSUxMTE5OTlFRUVVVVVlZWV1dXWFhYWRkZGZmZmhoaGpqamxsbG5ubnBwcHJycnV1dXp6en19fYd7b4+DdJGFdZKGeJOIdpWJeJeMepiKfJmNepmOfZyPfpqQfp2RfoKCgoODhISEhImJiYyMjI6OjpqOgJyPgZuSgZyRgZ2ShJ2Ugp6VhJ+YiZGRkZSUlJaWlpmZmZ6cmpycnKCTgKGThaGUgaCUhKOXiaOai6Obj6SYiKiZhqmaiKuciaydiq2fjKWek62gjqOgnqahmqWgnqigkqqila2hkqqjmq2jna6lnLChjrCikLOlkrKklLSlkbaplrOomrGonbWpmbirmbism7qunqCgoKKioqWlpa6noK6poqmpqa2sq62trbGoobKopLSrorauo7Sspbatqbmvobivpbiurbqypryxob2ypb+0pbqxqbixrLyzqr20qb20rrCwsLKysrS0tLa2tryzsL61sLi4uLq6ur29vcG3psC2qcG4q8K5rcW5rcC3sMK5scO5tMS7t8W9ssa9tMO9u8i9sci/ucDAv8fCvsjAtszCtsnBuMvDv8vGvczDvM3FvdDHv8HBwcTExMbGxsrDwMrEws3GwsvIwszIws3JxM/Mx8jIyM3Nzc/P0NDHw9HJwtHKxtLOx9TMxtTOydjPytXRytXSzdfUztjSzNjVztDQ0NLS0tTU1NbW1trX0NjY197Y1N/d19jY2Nra2t/d2Nzc3N7e3sPlyMjozMzn0c7o09Tr2N/g4OHe3OHg2+Lg3OHh4eTk5Obm5unm4+jo6Orq6uzr6+3s6+zs7O7u7vDt6fLx7vDw8PPy8PLy8vT09Pb29vD58fn39vj4+Pn6+Pr6+vz8/P39/v7+/P7+/gAAAO8A5NcAAAEAdFJOU////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wBT9wclAAAACXBIWXMAAA7DAAAOwwHHb6hkAAAAGXRFWHRTb2Z0d2FyZQBwYWludC5uZXQgNC4wLjE3M26fYwAAEMdJREFUaEPtWnt4G1dWvzN6YMnCL9mRbCHhWNHDkh0ntsRjC023QoopWYoTrYAFuqHLs8BmYUt52AIhYBfYUgpb26K2KdkWGgqUwhbFUFgs6LIhZSPLPGQS3lQKEiuDKg1MZ8j99pw7o7GUyibhY/mrx180d+7ce39z5jzuOeeG/CP9nNN//z35f0Ch//A2yl2K8u0vdpcfrF3Hbmx3UFGWd6CjvL9iXXleZz1SodCES317R6Y7rb5i5xLa3Da5eAwdNCrtjUDHyX2UVSMbsMx6Kv3mTbisGgZFedCQYX01nNBGJ1pz21COEp1JIwMZlvbscDtTA5LpXk2gGTPcc2QROuq0MtS3VmvSjMl6s2omq1SAvtoAMe4voSNTXVGmdzU6ByjyDbidt1qtIw3ZbV2iDbgtDBML9HipdL3gsZ6Fvq2RIR5QklavCCip/SUmD0AJ7X+dNKCwu7OEEL4u28gCuxVscE+IA5tOEoFLlYf7VRonDgFQLu8vcfxAlILfh5Shy4DSmIZmKAXk9xkBZct3UpDXUg7iTq3TvYDXzFCENAw44ZtLrcsMZZGtMFmmB6Nk8cUIrIi8wCRCpmEh0cD6lrkjDbjzEpTqTRP0IQojNxvHUE6yFXp27gQFeRHmw+HwKr0ZCfNkOrxFdyJwr1CcNmKhAUAptvpWOlGMd4SiyYXSHL5bnr1ywQjNNF1gcmmiXJbZm4NcmCbv83JnKMiLtJmtwNziuItjKNVs6qhLDygplz+bzW5MuiLZuAsIdex/haLIhUux2SAXRAG51EHH0tDcgvfXZWmIeOBG6r1LlM0BC1KC6djeWP9iuVyuUtHat0Pr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