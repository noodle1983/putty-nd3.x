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

Or Alipay:![avatar](https://raw.githubusercontent.com/noodle1983/private/master/qr_icon/noodle1983_ali.png)
Or Wechat:![avatar](https://raw.githubusercontent.com/noodle1983/private/master/qr_icon/noodle1983_wx.png)


Old Version Upgrade Alerts
============

The old config data **prior to putty-nd6.0**  will be upgraded , so please backup the settings to files or Google Driver incase bad thing happens.


RELEASE NOTE
============
--------------------------------
putty-nd6.6
--------------------------------
1. Bugs	

2. Other changes
	1. shift + left click to extend selection.
	2. ctrl + wheel to change the font size.
	3. a command editor dialog to prepare/save the scripts.
	4. sync the saved scripts to google drive.
	5. add line delay paste feature with the config item via Terminal -> Line Paste Delay.

--------------------------------
putty-nd6.5
--------------------------------
1. Bugs	
	1. putty-nd works wrong with pageant as the key is not syncd.
	2. putty-nd GUI broken at Windows 10. An walkaround is provided. The Windows native theme is disabled by default. It can be reopened via Global Settings -> Global Bugs in the session dialog.
	3. re-enable extend selection.

2. Other changes
	
	
--------------------------------
putty-nd6.4
--------------------------------
1. Bugs	
	1. crash on exit.

2. Other changes
	1. better ui to manage session on the google drive.
	2. configurable shortcuts keys via Global Settings in Config Dialog.
	3. auto reconnect feature and configurable via Session's Setting -> Connection. It is disabled by default.
	
	
3. Ongoing 

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
