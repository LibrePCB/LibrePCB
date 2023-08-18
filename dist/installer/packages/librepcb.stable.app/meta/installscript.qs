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
                                   "@TargetDir@/stable/bin/librepcb.exe",
                                   "@StartMenuDir@/LibrePCB.lnk");
        }

        if (systemInfo.kernelType === "linux") {
            component.addOperation("InstallIcons",
                                   "@TargetDir@/stable/share/icons");
            component.addOperation("CreateDesktopEntry",
                                   "librepcb-stable-from-installer.desktop",
                                   "Name=LibrePCB\n" +
                                   "Comment=Design Schematics and PCBs\n" +
                                   "GenericName=PCB Designer\n" +
                                   "Categories=Development;Engineering;Electronics;\n" +
                                   "Type=Application\n" +
                                   "Terminal=false\n" +
                                   "Icon=org.librepcb.LibrePCB\n" +
                                   "Exec=@TargetDir@/stable/bin/librepcb %U\n" +
                                   "MimeType=application/x-librepcb-project;");
            component.addOperation("Execute",
                                   "update-desktop-database",
                                   getShareDirectory() + "/applications");
        }
    } catch (e) {
        print(e);
    }
}
