from smtplib import SMTP
from email import MIMEText
from socket import sslerror

def send_email(fromAddr, passwd, toAddr, subject, body):
    server = SMTP('smtp.gmail.com',587)
    server.set_debuglevel(0)
    server.ehlo(fromAddr)
    server.starttls()
    server.ehlo(fromAddr)
    server.login(fromAddr,passwd)
    msg = MIMEText(body)
    msg['Subject'] = subject
    msg['From'] = fromAddr
    msg['To'] = toAddr
    server.sendmail(fromAddr,toAddr,msg.as_string() )
    server.quit()
    
