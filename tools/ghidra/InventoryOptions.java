// Use for the function inventory pass to bound heap usage on PE32 builds.
//@category T6
import ghidra.app.script.GhidraScript;

public class InventoryOptions extends GhidraScript {
    @Override
    public void run() {
        String option = "x86 Constant Reference Analyzer";
        if (!getCurrentAnalysisOptionsAndValues(currentProgram).containsKey(option)) {
            throw new IllegalStateException("Missing analysis option: " + option);
        }
        setAnalysisOption(currentProgram, option, "false");
        println("Disabled " + option + " for the inventory pass.");
    }
}
