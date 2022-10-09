import os
import time

while True:
    netstat = os.popen('netstat -ao').read()
    if str(12345) in netstat:  # if server agent is running
        pass
    else:
        os.system('python3 server_agent.py')
    time.sleep(1)
