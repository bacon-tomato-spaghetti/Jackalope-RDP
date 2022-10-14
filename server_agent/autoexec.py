import os
import subprocess
import time
from server_agent import HOST, PORT

while True:
    netstat = os.popen('netstat -ao').read()
    if f'{HOST}:{PORT}' in netstat:  # if server agent is running
        pass
    else:
        subprocess.run(['python3', 'server_agent.py'])
    time.sleep(1)
