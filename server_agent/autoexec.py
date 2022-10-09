import os
import time
from server_agent import PORT

while True:
    netstat = os.popen('netstat -ao').read()
    if str(PORT) in netstat:  # if server agent is running
        pass
    else:
        os.system('python3 server_agent.py')
    time.sleep(1)
