#  This program is free software; you can use it, redistribute it
#  and / or modify it under the terms of the GNU General Public License
#  (GPL) as published by the Free Software Foundation; either version 3
#  of the License or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program, in a file called gpl.txt or license.txt.
#   If not, write to the Free Software Foundation Inc.,
#   59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
import platform
from subprocess import check_output, CalledProcessError

Import("env")


def is_tool(name):
    cmd = "where" if platform.system() == "Windows" else "which"
    try:
        check_output([cmd, name])
        return True
    except:
        return False;


def build_ui():
    if is_tool("npm"):
        print("Attempting to build UI...")
        try:
            if platform.system() == "Windows":
                print(check_output(["npm.cmd", "install"]))
                print(check_output(["npm.cmd", "run", "build"]))
            else:
                print(check_output(["npm", "install"]))
                print(check_output(["npm", "run", "build"]))
        except OSError as e:
            print("Encountered error OSError building UI:", e)
            if e.filename:
                print("Filename is", e.filename)
            print("WARNING: Failed to build UI package Using pre-built page.")
        except CalledProcessError as e:
            print(e.output)
            print("Encountered error CalledProcessError building UI:", e)
            print("WARNING: Failed to build UI package. Using pre-built page.")
        except Exception as e:
            print("Encountered error", type(e).__name__, "building UI:", e)
            print("WARNING: Failed to build UI package. Using pre-built page.")


build_ui()