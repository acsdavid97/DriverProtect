"use strict";

function initializeScript()
{
    host.diagnostics.debugLog("***>; initializeScript was called\n");
    class processEntryVisualizer
    {
        get ImageName()
        {
            var processPointer = host.createPointerObject(this.Process.address, "nt", "_EPROCESS*");
            return processPointer.ImageFilePointer.FileName;
        }

        get Pid()
        {
            var processPointer = host.createPointerObject(this.Process.address, "nt", "_EPROCESS*");
            return processPointer.UniqueProcessId;
        }

        toString()
        {
            return this.ImageName.toString();
        }
    }

    class processCollectorVisualizer
    {
        *[Symbol.iterator]()
        {
            var listEntry = this.MonitoredProcessesHead.Flink;
            while(listEntry.address != this.MonitoredProcessesHead.address)
            {
                // this line assumes that the listentry is the first field in the monitored entry.
                var pointer = host.createPointerObject(listEntry.address, "SelfProtect", "_MONITORED_PROCESS_ENTRY*");
                yield pointer.dereference();
                listEntry = listEntry.Flink;
            }
        }
    }

    return [
        new host.apiVersionSupport(1, 3),
        new host.typeSignatureRegistration(processCollectorVisualizer, "_PROCESS_COLLECTOR_DATA"),
        new host.typeSignatureRegistration(processEntryVisualizer, "_MONITORED_PROCESS_ENTRY")
    ];
}

function invokeScript()
{
    host.diagnostics.debugLog("***>; invokeScript was called\n");
}
