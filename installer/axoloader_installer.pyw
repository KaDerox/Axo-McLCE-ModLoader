from importlib.resources import path
from logging import root
import tkinter as tk
from tkinter import Tk, PhotoImage, ttk
import getpass
from tkinter import filedialog
import time
import os
import shutil
import requests
import threading
import stat
import subprocess
from unittest import result
import json
import urllib.request
import sys
import winreg

def resource_path(relative_path):
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")

    return os.path.join(base_path, relative_path)

# Get the current user's username
username = getpass.getuser()

# Global variable to cache the changelog content
_changelog_cache = None

# Find cmake instead of hardcoding it 
def find_cmake():
    for drive in "CDEFGHIJKLMNOPQRSTUVWXYZ":
        path = rf"{drive}:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if os.path.exists(path):
            return path
    return None

advanced_settings = {
    "cmake_path": find_cmake() or "cmake",
    "vs_version": "VS 2022"
}

# Function to chose loader version
def loader_version():
    selected_option = loader_combo.get()

# Function to chose McLCE version
def version_selected():
    selected_option = version_combo.get()

# Function to fetch loader versions from an API and update the combobox with threading to avoid freezing the GUI
def fetch_loader_versions():
    try:
        response = requests.get("http://axoloader.eu/api/assets/versions.php", timeout=5)
        response.raise_for_status()
        data = response.json()
        versions = [v["name"] for v in data]
        root.after(0, lambda: loader_combo.config(values=versions))
        root.after(0, lambda: loader_combo.set(versions[0] if versions else "No versions available"))
    except requests.RequestException as e:
        print(f"Error fetching loader versions: {e}")

# Function to open a directory selection dialog
def browse_directory():
    directory = filedialog.askdirectory(
        initialdir = f"C:\\Users\\{username}\\"
        )
    if directory:
        directory_entry.delete(0, tk.END)
        directory_entry.insert(0, directory)

# Function to validate the selected directory
def smartcmd_validate_directory(path):
    sln_path = os.path.join(path, "CMakePresets.json")

    if os.path.exists(sln_path):
        return True , "Valid directory selected."
    else:
        return False , "Invalid directory. Please select a valid directory."

# Class to create a tooltip for a widget
class ToolTip:
    def __init__(self, widget, text):
        self.widget = widget
        self.text = text
        self.tooltip = None
        
        # Bind events to show and hide the tooltip
        self.widget.bind("<Enter>", self.show_tooltip)
        self.widget.bind("<Leave>", self.hide_tooltip)
    
    # Function to show the tooltip when the mouse enters the widget
    def show_tooltip(self, event=None):
        x, y, _, _ = self.widget.bbox("insert")
        x += self.widget.winfo_rootx() + 25
        y += self.widget.winfo_rooty() + 25
        
        # Create a tooltip window
        self.tooltip = tk.Toplevel(self.widget)
        self.tooltip.wm_overrideredirect(True)  
        self.tooltip.wm_geometry(f"+{x}+{y}")
        
        label = tk.Label(
            self.tooltip, 
            text=self.text, 
            background="#ffffff",
            relief="solid", 
            borderwidth=0.5,
            font=("Arial", 7, "bold"),
            padx=5,
            pady=3
        )
        label.pack()

    # Function to hide the tooltip when the mouse leaves the widget
    def hide_tooltip(self, event=None):
        if self.tooltip:
            self.tooltip.destroy()
            self.tooltip = None

# Advanced options functions
def advanced_options():
    advanced = tk.Toplevel(root)
    advanced.title("Axo McLCE ModLoader Advanced Settings")
    advanced.logo = PhotoImage(file=resource_path("assets/logo.png"))
    advanced.iconphoto(True, advanced.logo)
    advanced.geometry("500x200")
    advanced.resizable(False, False)
    advanced.lift()
    advanced.focus_force()
    advanced.grab_set()

    vs_selection_label = tk.Label(advanced, text="Select your Visual Studio version:")
    vs_selection_label.place(relx=0.2, rely=0.05, anchor=tk.CENTER)
    vs_combo = tk.ttk.Combobox(advanced, values=["VS 2022", "VS 2026"], state="readonly", width=12)
    vs_combo.set(advanced_settings["vs_version"])
    vs_combo.place(relx=0.12, rely=0.15, anchor=tk.CENTER)

    cmake_path_label = tk.Label(advanced, text="Select your CMake location:")
    cmake_path_label.place(relx=0.17, rely=0.27, anchor=tk.CENTER)
    cmake_path_entry = tk.Entry(advanced, width=75)
    cmake_path_entry.insert(0, advanced_settings["cmake_path"])
    cmake_path_entry.place(relx=0.48, rely=0.36, anchor=tk.CENTER)
    cmake_detect_button = tk.Button(advanced, text="Auto Detect", command=lambda: auto_detect())
    cmake_detect_button.place(relx=0.095, rely=0.49, anchor=tk.CENTER)

    advanced_done_button = tk.Button(advanced, text="Done", command=lambda: on_done())
    advanced_done_button.place(relx=0.9, rely=0.85, anchor=tk.CENTER)

    def on_done():
        advanced_settings["cmake_path"] = cmake_path_entry.get()
        advanced_settings["vs_version"] = vs_combo.get()
        advanced.destroy()

    def auto_detect():
        path = find_cmake()
        if path:
            cmake_path_entry.delete(0, tk.END)
            cmake_path_entry.insert(0, path)
        else:
            tk.messagebox.showerror("Not Found", "Could not find CMake automatically.")

# Function to handle the installation process
def install_smartcmd_modloader():
    McLCE_Version = version_combo.get()
    source = directory_entry.get()
    root.withdraw()  # Hide the main window during installation
    stop_thread = threading.Event()  # Event to signal the installation thread to stop
    if McLCE_Version == "McLCE Nightly By Smartcmd":
        destination = source
    else:
        destination = os.path.join(os.path.dirname(source), "Axo_McLCE_ModLoader")

    # Get the selected loader version, directory, and shortcut option
    selected_loader = loader_combo.get()
    selected_directory = directory_entry.get()
    create_shortcut = doShortcut.get() == 1

    # Function to create a shortcut 
    def create_shortcut_file():
        selected_build = loader_combo.get()
        file_exe = os.path.join(destination, "x64", selected_build, "Minecraft.Client.exe")

        if create_shortcut:
            desktop = os.path.join(os.path.expanduser("~"), "Desktop")
            shortcut_path = os.path.join(desktop, "AxoLoader.lnk")
            working_dir = os.path.dirname(file_exe)

            file_exe_ps = file_exe.replace("\\", "/")
            working_dir_ps = working_dir.replace("\\", "/")
            shortcut_path_ps = shortcut_path.replace("\\", "/")

            ps = (
                f'$ws = New-Object -ComObject WScript.Shell;'
                f'$s = $ws.CreateShortcut("{shortcut_path_ps}");'
                f'$s.TargetPath = "{file_exe_ps}";'
                f'$s.WorkingDirectory = "{working_dir_ps}";'
                f'$s.Save()'
            )
            subprocess.run(["powershell", "-Command", ps], check=True)
    
    def create_mods():
        selected_build = build_combo.get()
        if McLCE_Version == "McLCE Nightly By Smartcmd":
            destination_mod = os.path.join(source, "build", "windows64", "Minecraft.Client", selected_build)
        else:
            destination_mod = os.path.join(destination, "x64", selected_build)
        os.makedirs(os.path.join(destination_mod, 'mods'), exist_ok=True)

    # Function to remove read-only attribute from files and directories
    def remove_readonly(func, path, excinfo):
        os.chmod(path, stat.S_IWRITE)
        func(path)

    # Function to perform the installation (copying files, etc.)
    def perform_installation():
        if McLCE_Version == "McLCE By Smartcmd":
            try:
                if os.path.exists(destination):
                    shutil.rmtree(destination, onerror=remove_readonly)
                for dirpath, dirnames, filenames in os.walk(source):
                    if stop_thread.is_set():
                        return
                    rel = os.path.relpath(dirpath, source)
                    dest_dir = os.path.join(destination, rel)
                    os.makedirs(dest_dir, exist_ok=True)
                    for file in filenames:
                        shutil.copy2(os.path.join(dirpath, file), os.path.join(dest_dir, file))
            except Exception as e:
                err = str(e)
                root.after(0, lambda: tk.messagebox.showerror("Copy Error", f"Failed to copy files:\n{err}"))

    # Function to download the modloader files
    def download_modloader():
        build_version = loader_combo.get()
        if McLCE_Version == "McLCE Nightly By Smartcmd":
            target_dir = os.path.join(source, "Minecraft.Client", "Windows64")
        else:
            target_dir = os.path.join(destination, "Minecraft.Client", "Windows64")
        os.makedirs(target_dir, exist_ok=True)

        files = ["AxoModLoader.h", "AxoModLoader.cpp", "AxoAPI.h", "AxoAPI.cpp", "AxoItemImpl.cpp", "AxoBlockImpl.cpp", "AxoRecipeImpl.cpp", "AxoWorldGen.cpp", "AxoWorldGen.h", "AxoCropImpl.cpp", "AxoModelLoader.cpp", "AxoModelLoader.h", "AxoBiomeImpl.cpp", "AxoBlockImpl.h"]
        for file in files:
            try:
                response = requests.get(f"http://axoloader.eu/api/assets/modloader/{build_version}/{file}", timeout=10)
                response.raise_for_status()
                with open(os.path.join(target_dir, file), "wb") as f:
                    f.write(response.content)
            except requests.RequestException as e:
                err = str(e)
                root.after(0, lambda: tk.messagebox.showerror("Download Error", f"Failed to download {file}:\n{err}"))
                return

    # Functions to inject the modloader into the game
    def inject_modloader(filepath, search_line, inject_code):
        if not os.path.exists(filepath):
            root.after(0, lambda: tk.messagebox.showerror("Injection Error", f"File not found:\n{filepath}"))
            return
        with open(filepath, "r", encoding="utf-8") as file:
            content = file.read()
        if inject_code in content:
            print(f"Modloader already injected in {filepath}. Skipping injection.")
            return
        new_content = content.replace(search_line, search_line + "\n" + inject_code)
        with open(filepath, "w", encoding="utf-8") as file:
            file.write(new_content)

    def inject_modloader_replace(filepath, search_line, inject_code):
        if not os.path.exists(filepath):
            root.after(0, lambda: tk.messagebox.showerror("Injection Error", f"File not found:\n{filepath}"))
            return
        with open(filepath, "r", encoding="utf-8") as file:
            content = file.read()
        if inject_code in content:
            print(f"Modloader already injected in {filepath}. Skipping injection.")
            return
        new_content = content.replace(search_line, inject_code)
        with open(filepath, "w", encoding="utf-8") as file:
            file.write(new_content)

    def inject_modloader_before(filepath, search_line, inject_code):
        if not os.path.exists(filepath):
            root.after(0, lambda: tk.messagebox.showerror("Injection Error", f"File not found:\n{filepath}"))
            return
        with open(filepath, "r", encoding="utf-8") as file:
            content = file.read()
        if inject_code in content:
            print(f"Modloader already injected in {filepath}. Skipping injection.")
            return
        new_content = content.replace(search_line, inject_code + "\n" + search_line)
        with open(filepath, "w", encoding="utf-8") as file:
            file.write(new_content)

    # Function to inject code into the .vcxproj file to include the new modloader files
    def inject_cmakelists(filepath, search_line, inject_code):
        if not os.path.exists(filepath):
            root.after(0, lambda: tk.messagebox.showerror("CMakeLists Error", f"File not found:\n{filepath}"))
            return
        with open(filepath, "r", encoding="utf-8") as file:
            content = file.read()
        if inject_code in content:
            return
        if search_line not in content:
            root.after(0, lambda: tk.messagebox.showerror("CMakeLists Error", f"Anchor not found:\n{search_line}"))
            return
        new_content = content.replace(search_line, search_line + "\n" + inject_code)
        with open(filepath, "w", encoding="utf-8") as file:
            file.write(new_content)

    def check_build_environment():
        sdk_path = r"C:\Program Files (x86)\Windows Kits\10\Include"
        if not os.path.exists(sdk_path) or not os.listdir(sdk_path):
            root.after(0, lambda: tk.messagebox.showerror(
                "Missing Dependencies",
                "Windows SDK not found.\n\n"
                "Please install it via Visual Studio Installer:\n"
                "- Desktop development with C++\n"
                "- Windows 10/11 SDK"
            ))
            return False
        return True

    # Function to compile the project using MSBuild
    def compile_project():
        if useAutocompiler.get() == 0:
            return True

        build_type = build_combo.get()
        if stop_thread.is_set():
            return False

        cmake_path = advanced_settings["cmake_path"]
        base = source if McLCE_Version == "McLCE Nightly By Smartcmd" else destination

        vs_version = advanced_settings["vs_version"]
        vcvars = rf"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
        if not os.path.exists(vcvars):
            root.after(0, lambda: tk.messagebox.showerror("VS Not Found", "vcvarsall.bat not found.\nCheck Advanced Settings."))
            return False

        preset = "windows64-debug" if build_type == "Debug" else "windows64-release"

        configure_cmd = f'"{vcvars}" x64 && "{cmake_path}" --preset windows64'
        build_cmd = f'"{vcvars}" x64 && "{cmake_path}" --build --preset {preset} --target Minecraft.Client'

        for cmd in [configure_cmd, build_cmd]:
            result = subprocess.run(
                cmd,
                capture_output=True,
                encoding="utf-8",
                errors="replace",
                cwd=base,
                shell=True
            )
            if result.returncode != 0:
                output = (result.stdout + result.stderr)[-1000:]
                root.after(0, lambda o=output: tk.messagebox.showerror("Compilation Failed", f"CMake build failed.\n\n{o}"))
                return False
        return True

    # Function to handle cancellation of the installation
    def cancel_instalation():
        stop_thread.set()
        root.deiconify()
        instalation.destroy()
        progress.stop()

    # Function to update the progress bar and display messages during installation
    def update_progress():
        base = source if McLCE_Version == "McLCE Nightly By Smartcmd" else destination
        if stop_thread.is_set():
            return  # Exit the installation if the cancel button was clicked
        if not instalation.winfo_exists():
            return  # Exit if the installation window has been closed
        vcxproj = os.path.join(base, "Minecraft.Client", "Minecraft.Client.vcxproj")
        mc_cpp = os.path.join(base, "Minecraft.Client", "Windows64", "Windows64_Minecraft.cpp")
        creative_menu = os.path.join(base, "Minecraft.Client", "Common", "UI", "IUIScene_CreativeMenu.h")
        itemh = os.path.join(base, "Minecraft.World", "Item.h")
        pstmcpp = os.path.join(base, "Minecraft.Client", "PreStitchedTextureMap.cpp")
        pstmh = os.path.join(base, "Minecraft.Client", "PreStitchedTextureMap.h")
        mcpp = os.path.join(base, "Minecraft.Client", "Minecraft.cpp")
        uiscene_mm_cpp = os.path.join(base, "Minecraft.Client", "Common", "UI", "UIScene_MainMenu.cpp")
        recipes = os.path.join(base, "Minecraft.World", "Recipes.h")
        biome_decorator = os.path.join(base, "Minecraft.World", "BiomeDecorator.cpp")
        hell_random = os.path.join(base, "Minecraft.World", "HellRandomLevelSource.cpp")
        hell_flat = os.path.join(base, "Minecraft.World", "HellFlatLevelSource.cpp")
        survival_mode = os.path.join(base, "Minecraft.Client", "SurvivalMode.cpp")
        tile_h = os.path.join(base, "Minecraft.World", "Tile.h")
        biome_intit_layer = os.path.join(base, "Minecraft.World", "BiomeInitLayer.cpp")
        tile_renderer = os.path.join(base, "Minecraft.Client", "TileRenderer.cpp")
        dye_powder_item = os.path.join(base, "Minecraft.World", "DyePowderItem.cpp")
        steps = [
            (40, "Copying files... (This may take a bit)", lambda: perform_installation()),
            (50, "Downloading modloader...", lambda: download_modloader()),
            (55, "Downloading additional libraries...", None),
            (60, "Injecting modloader...", lambda:[
                inject_modloader(mc_cpp, '#include "..\\Textures.h"', '#include "AxoModLoader.h"'),
                inject_modloader(mc_cpp, 'app.InitialiseTips();', '\tAxoModLoader_Init("mods");'),
                inject_modloader(mc_cpp, 'RenderManager.StartFrame();', '\tAxoModLoader_Tick();'),
                inject_modloader(mc_cpp, '//\tapp.Uninit();', '\tAxoModLoader_Shutdown();'),
                inject_modloader(creative_menu, 'static void staticCtor();', '\tstatic void AxoAddToGroup(ECreative_Inventory_Groups group, shared_ptr<ItemInstance> item) { categoryGroups[group].push_back(item); }'),
                inject_modloader_replace(itemh, 'std::wstring getName();', 'virtual std::wstring getName();'),
                inject_modloader_before(mc_cpp, 'Minecraft::main();', 'AxoModLoader_PreInit("mods");'),
                inject_modloader(pstmcpp, '#include "ClockTexture.h"', '#include <vector>'),
                inject_modloader(pstmcpp, 'const wstring PreStitchedTextureMap::NAME_MISSING_TEXTURE = L"missingno";', 'std::vector<PreStitchedTextureMap::AxoIconSlot> PreStitchedTextureMap::s_axoPendingIcons;'),
                inject_modloader(pstmcpp, 'texturesByName[L"compassP3"] = compass;', '\n\t\tfor (auto& e : s_axoPendingIcons) {\n\t\t\tADD_ICON(e.row, e.col, e.name)\n\t\t}'),
                inject_modloader_before(pstmh, '};', '\tstruct AxoIconSlot { wstring name; int row, col; };\n' '\tstatic vector<AxoIconSlot> s_axoPendingIcons;\n' '\tstatic void AxoQueueIcon(const wstring& name, int row, int col) {\n' '\t\ts_axoPendingIcons.push_back({name, row, col});\n' '\t}\n' '\tstruct AxoTerrainIconSlot { wstring name; int row, col; };\n' '\tstatic vector<AxoTerrainIconSlot> s_axoTerrainPendingIcons;\n' '\tstatic void AxoQueueTerrainIcon(const wstring& name, int row, int col) {\n' '\t\ts_axoTerrainPendingIcons.push_back({name, row, col});\n' '\t}\n'),
                inject_modloader(pstmh, 'class BufferedImage;', 'class TexturePack;'),
                inject_modloader(mcpp, 'MinecraftWorld_RunStaticCtors();', '\tAxoModLoader_MidInit();'),
                inject_modloader(mcpp, '#include "DLCTexturePack.h"', '#include "Windows64/AxoModLoader.h"'),
                inject_modloader(pstmcpp, 'float vertRatio = 1.0f/32.0f;', '\n\t\tfor (auto& e : s_axoTerrainPendingIcons) {' '\n\t\t\tADD_ICON(e.row, e.col, e.name)' '\n\t\t}'),
                inject_modloader(pstmcpp, 'std::vector<PreStitchedTextureMap::AxoIconSlot> PreStitchedTextureMap::s_axoPendingIcons;', 'std::vector<PreStitchedTextureMap::AxoTerrainIconSlot> PreStitchedTextureMap::s_axoTerrainPendingIcons;'),
                inject_modloader(uiscene_mm_cpp, 'PIXEndNamedEvent();', '\n\n\t\tMinecraft *pMinecraft = Minecraft::GetInstance();\n\t\tFont *font = pMinecraft->font;\n\t\tCustomDrawData *cdd = ui.setupCustomDraw(this, region);\n\t\tdelete cdd;\n\t\tglDisable(GL_CULL_FACE);\n\t\tglDisable(GL_DEPTH_TEST);\n\t\tglPushMatrix();\n\t\tfloat scale = m_fScreenWidth / m_fRawWidth;\n\t\tfloat x = (-m_fRawWidth  / 2.2f) + 2.0f;\n\t\tfloat y = ( m_fRawHeight / 9.0f) - 10.0f;\n\t\tglTranslatef(x * scale, y * scale, 0);\n\t\tglScalef(scale, scale, scale);\n\t\tfont->drawShadow(L"AxoLoader v1.0.7", 0, 0, 0xAAAAAA);\n\t\tglPopMatrix();\n\t\tglEnable(GL_DEPTH_TEST);\n\t\tui.endCustomDraw(region);'),
                inject_modloader_replace(recipes, "private:\n\tvoid buildRecipeIngredientsArray();", "public:\n\tvoid buildRecipeIngredientsArray();"),
                inject_modloader(biome_decorator, '#include "stdafx.h"', '#include "..\\Minecraft.Client\\Windows64\\AxoWorldGen.h"'),
                inject_modloader(biome_decorator, 'level->setInstaTick(false);', '\tAxoWorldGen_Decorate(level, random, biome, xo, zo);'),
                inject_modloader(hell_random, '#include "stdafx.h"', '#include "..\\Minecraft.Client\\Windows64\\AxoWorldGen.h"'),
                inject_modloader(hell_random, 'app.processSchematics(parent->getChunk(xt,zt));', '\tAxoWorldGen_Decorate(level, pprandom, level->getBiome(xo + 8, zo + 8), xo, zo);'),
                inject_modloader(hell_flat, '#include "stdafx.h"', '#include "..\\Minecraft.Client\\Windows64\\AxoWorldGen.h"'),
                inject_modloader(hell_flat, 'app.processSchematics(parent->getChunk(xt,zt));', '\tAxoWorldGen_Decorate(level, pprandom, level->getBiome(xo + 8, zo + 8), xo, zo);'),
                inject_modloader_replace(survival_mode, 'if (changed && couldDestroy) \n\t{\n\t\tTile::tiles[t]->playerDestroy(minecraft->level, minecraft->player, x, y, z, data);\n\t}', 'if (changed && (couldDestroy || Tile::tiles[t]->isAxoCanBeBrokenByHand())) \n\t{\n\t\tTile::tiles[t]->playerDestroy(minecraft->level, minecraft->player, x, y, z, data);\n\t}'),
                inject_modloader(tile_h, 'int getFaceFlags(LevelSource *level, int x, int y, int z);', '\tvirtual bool isAxoCanBeBrokenByHand() { return false; }'),
                inject_modloader(biome_intit_layer, '#include "BiomeInitLayer.h"', '#include "..\\Minecraft.Client\\Windows64\\AxoWorldGen.h"'),
                inject_modloader(biome_intit_layer, 'startBiomes[6] = Biome::jungle;\n\t}', '\tconst auto& customBiomes = AxoWorldGen_GetCustomBiomes();\n\tif (!customBiomes.empty()) {\n\t\tint newLen = startBiomes.length + (int)customBiomes.size();\n\t\tBiomeArray extended(newLen);\n\t\tfor (int i = 0; i < startBiomes.length; i++)\n\t\t\textended[i] = startBiomes[i];\n\t\tfor (int i = 0; i < (int)customBiomes.size(); i++)\n\t\t\textended[startBiomes.length + i] = Biome::biomes[customBiomes[i].biomeId];\n\t\tdelete[] startBiomes.data;\n\t\tstartBiomes = extended;\n\t}' ),
                inject_modloader(tile_renderer, '\tcase Tile::SHAPE_HOPPER:\n\t\tretVal = tesselateHopperInWorld(tt, x, y, z);\n\t\tbreak;', '\tcase SHAPE_AXO_CUSTOM_MODEL:\n\t\tnoCulling = true;\n\t\tretVal = AxoModelLoader_Tessellate(this, tt, x, y, z);\n\t\tnoCulling = false;\n\t\tbreak;'),
                inject_modloader_before(tile_renderer, 'if ( renderShape == SHAPE_CROSS_TEXTURE ) return true;', '\tif ( renderShape == SHAPE_AXO_CUSTOM_MODEL ) return true;'),
                inject_modloader(tile_renderer, 't->setMipmapEnable( Tile::mipmapEnable[tile->id] );	// 4J added', '\n\tif (shape == SHAPE_AXO_CUSTOM_MODEL ||\n\t\t(shape == Tile::SHAPE_BLOCK && AxoModelLoader_HasModel(tile->id)) ||\n\t\tAxoModelLoader_HasModel(tile->id)) {\n\t\tglTranslatef(-0.5f, -0.5f, -0.5f);\n\t\tAxoModelLoader_TessellateForRenderTile(this, tile, brightness);\n\t\tt->setMipmapEnable(true);\n\t\treturn;\n\t}'),
                inject_modloader(tile_renderer, '#include "stdafx.h"', '#include "Windows64/AxoModelLoader.h"'),
                inject_modloader(tile_renderer, 'if ( renderShape == Tile::SHAPE_ANVIL) return true;', '\tif ( renderShape == SHAPE_AXO_CUSTOM_MODEL ) return true;'),
                inject_modloader(dye_powder_item, '#include "Material.h"', '#include "CropTile.h"'),
                inject_modloader(dye_powder_item,  'else if (tile == Tile::wheat_Id) \n\t{\n\t\tif (level->getData(x, y, z) == 7) return false;\n\t\tif(!bTestUseOnOnly)\n\t\t{\t\n\t\t\tif (!level->isClientSide) \n\t\t\t{\n\t\t\t\tstatic_cast<CropTile *>(Tile::tiles[tile])->growCrops(level, x, y, z);\n\t\t\t\titemInstance->count--;\n\t\t\t}\n\t\t}\n\t\treturn true;\n\t}', 'else if (tile == Tile::wheat_Id) \n\t{\n\t\tif (level->getData(x, y, z) == 7) return false;\n\t\tif(!bTestUseOnOnly)\n\t\t{\t\n\t\t\tif (!level->isClientSide) \n\t\t\t{\n\t\t\t\tstatic_cast<CropTile *>(Tile::tiles[tile])->growCrops(level, x, y, z);\n\t\t\t\titemInstance->count--;\n\t\t\t}\n\t\t}\n\t\treturn true;\n\t}\n\telse if (CropTile* crop = dynamic_cast<CropTile*>(Tile::tiles[tile]))\n\t{\n\t\tif (level->getData(x, y, z) == 7) return false;\n\t\tif (!bTestUseOnOnly)\n\t\t{\n\t\t\tif (!level->isClientSide)\n\t\t\t{\n\t\t\t\tcrop->growCrops(level, x, y, z);\n\t\t\t\titemInstance->count--;\n\t\t\t}\n\t\t}\n\t\treturn true;\n\t}'),
            ]),
            (65, "Setting up dependencies...", lambda: [
        inject_cmakelists(
            os.path.join(base, "Minecraft.Client", "cmake", "sources", "Windows.cmake"),
            '  "${BASE_DIR}/Windows64_Minecraft.cpp"',
            '  "${BASE_DIR}/AxoAPI.cpp"\n  "${BASE_DIR}/AxoModLoader.cpp"\n  "${BASE_DIR}/AxoItemImpl.cpp"\n  "${BASE_DIR}/AxoBlockImpl.cpp"\n  "${BASE_DIR}/AxoRecipeImpl.cpp"\n  "${BASE_DIR}/AxoWorldGen.cpp"\n  "${BASE_DIR}/AxoCropImpl.cpp"\n  "${BASE_DIR}/AxoModelLoader.cpp"\n  "${BASE_DIR}/AxoBiomeImpl.cpp"'
        ),
        inject_cmakelists(
            os.path.join(base, "Minecraft.Client", "cmake", "sources", "Windows.cmake"),
            '  "${BASE_DIR}/KeyboardMouseInput.h"',
            '  "${BASE_DIR}/AxoAPI.h"\n  "${BASE_DIR}/AxoModLoader.h"\n  "${BASE_DIR}/AxoWorldGen.h"\n  "${BASE_DIR}/AxoModelLoader.h"\n  "${BASE_DIR}/AxoBlockImpl.h"'
        ),
        ]),
            (70, "Setting up project...", lambda: create_mods()),
            (80, "Compiling... (This may take a bit)",  lambda: compile_project()),
            (90, "Creating shortcut...", lambda: create_shortcut_file()),
            (100, "Installation complete!", None)
        ]

        progress.start()
        for value, message, action in steps:
            if stop_thread.is_set():
                return  # Exit the installation if the cancel button was clicked
            wait_label.config(text=message)
            if action:
                action()

        progress.stop()
        root.after(0, lambda: [
            instalation.destroy(),
            tk.messagebox.showinfo("Installation Complete", "McLCE ModLoader has been installed successfully!"),
            root.deiconify()
        ])

    # Installation logic goes here
    instalation = tk.Toplevel(root)
    instalation.title("Installing Axo McLCE ModLoader")
    instalation.logo = PhotoImage(file=resource_path("assets/logo.png"))
    instalation.iconphoto(True, instalation.logo)
    instalation.geometry("500x200")
    instalation.resizable(False, False)

    # Center the installation window on the screen
    instalation.update_idletasks()
    x = (instalation.winfo_screenwidth() // 2) - (instalation.winfo_width() // 2)
    y = (instalation.winfo_screenheight() // 2) - (instalation.winfo_height() // 2)
    instalation.geometry(f"+{x}+{y}")

    # Display installation information
    instalation_label = tk.Label(instalation, text="Installing Axo McLCE ModLoader...", font=("Arial", 12, "bold"))
    instalation_label.pack(pady=10)
    instalation_label.place(relx=0.5, rely=0.2, anchor=tk.CENTER)

    # Destination path for the installation
    destination = os.path.join(os.path.dirname(directory_entry.get()), "Axo_McLCE_ModLoader")
    
    # Information label to show selected options
    info_label = tk.Label(instalation, text=f"Installing Axo McLCE ModLoader version: {selected_loader}\n Instalation directory: {selected_directory}\n Output directory: {destination}", font=("Arial", 10))
    info_label.pack(pady=20)
    info_label.place(relx=0.5, rely=0.4, anchor=tk.CENTER)

    # Wait label to inform the user that the installation is in progress
    wait_label = tk.Label(instalation, text="Please wait while the installation is in progress...", font=("Arial", 8, "italic"))
    wait_label.pack(pady=10)
    wait_label.place(relx=0.5, rely=0.6, anchor=tk.CENTER)

    # Progress bar to show installation progress
    progress = ttk.Progressbar(instalation, orient="horizontal", length=300, mode="indeterminate")
    progress.pack(pady=20)
    progress.place(relx=0.5, rely=0.7, anchor=tk.CENTER)

    # Cancel button to close the installation window
    cancel_button = tk.Button(instalation, text="Cancel", command= lambda:cancel_instalation())
    cancel_button.pack(pady=10)
    cancel_button.place(relx=0.5, rely=0.85, anchor=tk.CENTER)
    instalation.protocol("WM_DELETE_WINDOW", lambda: cancel_instalation())


    # Start the installation process in a separate thread to avoid freezing the GUI
    t = threading.Thread(target=update_progress, daemon=True)
    t.start()

# Function to display the changelog of loader versions
def changelog():
    global _changelog_cache

    changelog_win= tk.Toplevel(root)
    changelog_win.title("Axo McLCE ModLoader Versions - Changelog")
    changelog_win.logo = PhotoImage(file=resource_path("assets/logo.png"))
    changelog_win.iconphoto(True, changelog_win.logo)
    changelog_win.geometry("410x500")
    changelog_win.resizable(False, False)
    scrollbar = tk.Scrollbar(changelog_win)
    scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    text_area = tk.Text(changelog_win, yscrollcommand=scrollbar.set, state="disabled")
    text_area.pack(fill=tk.BOTH, expand=True)
    scrollbar.config(command=text_area.yview)

    text_area.insert(tk.END, "Loading changelog...\n\n")
    text_area.config(state="disabled")

    stop_chanelog_thread = threading.Event()  # Event to signal the changelog thread to stop

    # Function to fetch the changelog from an API and update the text area with threading to avoid freezing the GUI
    def fetch_changelog():
        global _changelog_cache
        if _changelog_cache is not None:
            changelog_win.after(0, lambda: update_changelog(_changelog_cache))
            return
        try:
            response = requests.get("http://axoloader.eu/api/assets/changelog.txt", timeout=5)
            response.raise_for_status()
            _changelog_cache = response.text
        except Exception as e:
            _changelog_cache = f"Error fetching changelog: {e}"

        changelog_win.after(0, lambda: update_changelog(_changelog_cache))

    # Function to update the changelog text area with the fetched content
    def update_changelog(content):
        if not changelog_win.winfo_exists():
            return
        text_area.config(state="normal")
        text_area.delete(1.0, tk.END)
        text_area.insert(tk.END, content)
        text_area.config(state="disabled")
    
    def on_close():
        stop_chanelog_thread.set()  # Signal the changelog thread to stop
        changelog_win.destroy()
    changelog_win.protocol("WM_DELETE_WINDOW", on_close)
    threading.Thread(target=fetch_changelog, daemon=True).start()

# Create the main application window
root = Tk()
root.title("Axo McLCE ModLoader - Installer")
root.geometry("500x280")
root.logo = PhotoImage(file=resource_path("assets/logo.png"))
root.iconphoto(True, root.logo)
root.resizable(False, False)

# Variable to track the state of the shortcut checkbox
doShortcut = tk.IntVar()

useAutocompiler = tk.IntVar(value=1)

# Create a section for directory selection
directory_label = tk.Label(root, text="Select the download directory:")
directory_label.pack(pady=10)
directory_label.place(relx=0.2068, rely=0.63, anchor=tk.CENTER)
directory_entry = tk.Entry(root, width=50)
directory_entry.insert(0, f"C:\\Users\\{username}\\source\\repos\\MinecraftConsoles")
directory_entry.pack(pady=5)
directory_entry.place(relx=0.35, rely=0.7, anchor=tk.CENTER)
browse_button = tk.Button(root, text="...", command=browse_directory, width=2)
browse_button.pack(pady=10)
browse_button.place(relx=0.69, rely=0.7, anchor=tk.CENTER)
info_icon = tk.Label(root, text="ⓘ", font=("Arial", 10, "bold"), fg="black", cursor="hand2")
info_icon.pack(pady=20)
info_icon.place(relx=0.75, rely=0.7, anchor=tk.CENTER)
ToolTip(info_icon, "Select the directory where\n are the decompiled files of the game.\n NOT the game directory itself,\n but the decompiled files.")

# Label for confirmation
ready_label = tk.Label(root, text="If you are ready click Install!", font=("Arial", 6, "bold"))
ready_label.pack(pady=10)
ready_label.place(relx=0.84, rely=0.83, anchor=tk.CENTER)

# Loader version selection
loader_label = tk.Label(root, text="Select a loader version:")
loader_label.pack(pady=10)
loader_label.place(relx=0.1689, rely=0.48, anchor=tk.CENTER)
loader_combo = tk.ttk.Combobox(root, values=["Fetching Data...", "Fetching Data...", "Fetching Data..."], state="readonly", width=20)
loader_combo.pack(pady=10)
loader_combo.place(relx=0.188, rely=0.55, anchor=tk.CENTER)
loader_combo.set("Fetching Data...")
loader_combo.bind("<<ComboboxSelected>>", loader_version)

# Loader changelog tooltip
loader_info_icon = tk.Button(root, text="ⓘ", font=("Arial", 8, "bold"), fg="black", cursor="hand2", command=changelog, width=1, height=1)
loader_info_icon.pack(pady=20)
loader_info_icon.place(relx=0.022, rely=0.55, anchor=tk.CENTER)
ToolTip(loader_info_icon, "Click to view the changelog of loader versions.")

# McLCE Version selection
version_label = tk.Label(root, text="Select a McLCE version:")
version_label.pack(pady=10)
version_label.place(relx=0.48, rely=0.48, anchor=tk.CENTER)
version_combo = tk.ttk.Combobox(root, values=["McLCE By Smartcmd", "McLCE Nightly By Smartcmd"], state="readonly", width=20)
version_combo.pack(pady=10)
version_combo.place(relx=0.5, rely=0.55, anchor=tk.CENTER)
version_combo.set("McLCE By Smartcmd")
version_combo.bind("<<ComboboxSelected>>", version_selected)

# Shortcut checkbox
tk.Checkbutton(root, text="Create desktop shortcut?", variable=doShortcut).place(relx=0.1925, rely=0.8, anchor=tk.CENTER)

tk.Checkbutton(root, text="Use Autocompiler?", variable=useAutocompiler).place(relx=0.5, rely=0.8, anchor=tk.CENTER)

# Center the window on the screen
root.update_idletasks()
x = (root.winfo_screenwidth() // 2) - (root.winfo_width() // 2)
y = (root.winfo_screenheight() // 2) - (root.winfo_height() // 2)
root.geometry(f"+{x}+{y}")

# Create welcome label and additional info label
welcome_label = tk.Label(root, text="Welcome to Axo McLCE ModLoader Installer!", font=("Arial", 12, "bold"))
welcome_label.pack(pady=10)
welcome_label.place(relx=0.5, rely=0.15, anchor=tk.CENTER)
welcome_label_info = tk.Label(root, text="This installer will guide you through the installation process.\n Backup your game files before proceeding.\n Remember this modloader is in beta and may have some issues.", font=("Arial", 8))
welcome_label_info.pack(pady=5)
welcome_label_info.place(relx=0.5, rely=0.3, anchor=tk.CENTER)

# Function to handle the installation process when the Install button is clicked
def on_install():
    McLCE_Version = version_combo.get()
    if McLCE_Version == "McLCE By Smartcmd":
        path = directory_entry.get()
        valid, message = smartcmd_validate_directory(path)
        if not valid:
            tk.messagebox.showerror("Invalid Directory", message)
        if valid:
            install_smartcmd_modloader()
    if McLCE_Version == "McLCE Nightly By Smartcmd":
        path = directory_entry.get()
        valid, message = smartcmd_validate_directory(path)
        if not valid:
            tk.messagebox.showerror("Invalid Directory", message)
        if valid:
            install_smartcmd_modloader()


# Install button
install_button = tk.Button(root, text="Install", command=lambda: on_install())
install_button.pack(pady=10)
install_button.place(relx=0.9, rely=0.9, anchor=tk.CENTER)
install_cancel_button = tk.Button(root, text="Cancel", command=root.destroy)
install_cancel_button.pack(pady=10)
install_cancel_button.place(relx=0.8, rely=0.9, anchor=tk.CENTER)

# Debug or release label & button
build_label = tk.Label(root, text="Select build type:")
build_label.pack(pady=10)
build_label.place(relx=0.765, rely=0.48, anchor=tk.CENTER)
build_combo = tk.ttk.Combobox(root, values=["Debug", "Release"], state="readonly", width=12)
build_combo.set("Debug")
build_combo.pack(pady=10)
build_combo.place(relx=0.767, rely=0.55, anchor=tk.CENTER)

developer_label = tk.Label(root, text="Developed by KaDerox :D", font=("Arial", 6, "italic"))
developer_label.pack(pady=10)
developer_label.place(relx=0.5, rely=0.95, anchor=tk.CENTER)

advanced_button = tk.Button(root, text="Advanced", command=lambda: advanced_options())
advanced_button.place(relx=0.1, rely=0.9, anchor=tk.CENTER)

threading.Thread(target=fetch_loader_versions, daemon=True).start()
root.mainloop()
