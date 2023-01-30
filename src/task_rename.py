import os
from shutil import copy2

class RenameTask:
    def __init__(self, old_name, new_name):
        self.old_name = old_name
        self.new_name = new_name

    def execute(self):
        if os.path.exists(self.old_name):
            copy2(self.old_name, self.new_name)
            os.remove(self.old_name)
    
    def cleanup(self):
        if os.path.exists(self.new_name):
            copy2(self.new_name, self.old_name)
            os.remove(self.new_name)
