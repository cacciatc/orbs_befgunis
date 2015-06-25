require 'posix_mq'
require 'sequel'
require 'date'

DB = Sequel.connect(ENV['DATABASE_URL'])

$PRGS = DB[:prgs] 

mq = POSIX_MQ.new "/orbs-befungis-responses", :r

while true
  msg = mq.receive
  
  payload = msg[0]
  
  code = payload[0][0]
  uuid = payload[1..payload.length]
  
  if code == "\x01"
    #puts "#{uuid} is born"
    $PRGS.insert :created_at => DateTime.now, :uuid => uuid
  elsif code == "\x00"
    #puts "#{uuid} has died"
    $PRGS.where(:uuid => uuid).update :died_at => DateTime.now
  end
end
