function Component()
{
    // Default constructor
}

Component.prototype.createOperations = function()
{
    // Call default implementation
    component.createOperations();
    
    // Add custom operations
    component.addOperation("CreateShortcut", 
        "@TargetDir@/psio-assist.exe", 
        "@StartMenuDir@/PSIO Assist EX.lnk", 
        "workingDirectory=@TargetDir@");
        
    component.addOperation("CreateShortcut", 
        "@TargetDir@/psio-assist.exe", 
        "@DesktopDir@/PSIO Assist EX.lnk", 
        "workingDirectory=@TargetDir@");
}
