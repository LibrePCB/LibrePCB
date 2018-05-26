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
            component.addOperation("CreateShortcut",
                                   "@TargetDir@/librepcb-maintenance.exe",
                                   "@StartMenuDir@/LibrePCB Maintenance Tool.lnk",
                                   " --updater");
        }

        if (systemInfo.kernelType === "linux") {
            component.addOperation("CreateDesktopEntry",
                                   "librepcb-maintenance.desktop",
                                   "Name=LibrePCB Maintenance Tool\n" +
                                   "Comment=Manage the LibrePCB Installation\n" +
                                   "Categories=Utility;\n" +
                                   "Type=Application\n" +
                                   "Terminal=false\n" +
                                   "Exec=@TargetDir@/librepcb-maintenance --updater");
            component.addOperation("Execute",
                                   "update-desktop-database",
                                   getShareDirectory() + "/applications");
        }
    } catch (e) {
        print(e);
    }
}
