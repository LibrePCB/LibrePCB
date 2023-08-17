function Component() {
}

function isRoot() {
    var id = installer.execute("/usr/bin/id", new Array("-u"))[0];
    id = id.replace(/(\r\n|\n|\r)/gm,"");
    return (id === "0");
}

function getShareDirectory() {
    if (isRoot()) {
        return "/usr/local/share";
    } else {
        return "@HomeDir@/.local/share"
    }
}

Component.prototype.createOperations = function() {
    component.createOperations();

    try {
        if (systemInfo.productType === "windows") {
            // DON'T REMOVE THE QUOTES AROUND @TargetDir! It took me TWO HOURS
            // to find out that this completely crappy operating system silently
            // passes fucking 8.3 DOS filenames as argv if the quotes are
            // missing!!! It's 2018, we want LONG FILENAMES!!! Fucking crap...
            // https://blogs.msdn.microsoft.com/oldnewthing/20041020-00/?p=37523
            // @Microsoft: Don't write blogs about that, fix this crappy shit!!!
            component.addOperation("RegisterFileType",
                                   "lpp",
                                   "\"@TargetDir@\\nightly\\bin\\librepcb.exe\" \"%1\"",
                                   "LibrePCB Project",
                                   "text/plain",
                                   "\"@TargetDir@\\nightly\\bin\\librepcb.exe\"",
                                   "ProgId=LibrePCB.lpp");
            component.addOperation("RegisterFileType",
                                   "lppz",
                                   "\"@TargetDir@\\nightly\\bin\\librepcb.exe\" \"%1\"",
                                   "LibrePCB Project Archive",
                                   "application/zip",
                                   "\"@TargetDir@\\nightly\\bin\\librepcb.exe\"",
                                   "ProgId=LibrePCB.lppz");
        }

        if (systemInfo.kernelType === "linux") {
            component.addOperation("Mkdir",
                                   getShareDirectory() + "/mime/packages");
            component.addOperation("Copy",
                                   "@TargetDir@/registerfileextensions/mime/librepcb-from-installer.xml",
                                   getShareDirectory() + "/mime/packages/librepcb-from-installer.xml");
            component.addOperation("Execute",
                                   "update-mime-database",
                                   getShareDirectory() + "/mime");
        }
    } catch (e) {
        print(e);
    }
}
