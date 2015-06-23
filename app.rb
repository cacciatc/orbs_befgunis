require 'sinatra'
require 'posix_mq'

configure do
  $MQ = POSIX_MQ.new "/orbs-befungis-requests", :w
end

get '/' do
  send_file './public/pages/index.html'
end

post '/api/fungals' do
  uuid = SecureRandom.base64(10)

  if(not params["prg"].nil? and params["prg"].length > 0)
    prg = params["prg"]
    
    $MQ << "#{uuid}#{prg}"
  end
end
