require 'sinatra'
require 'posix_mq'
require 'sequel'
require 'json'

configure do
  $MQ = POSIX_MQ.new "/orbs-befungis-requests", :w
  $DB = Sequel.connect(ENV['DATABASE_URL'])
  $PRGS = $DB[:prgs]
end

get '/' do
  send_file './public/pages/index.html'
end

# prg query by id
get '/api/fungals/:uuid' do |uuid|
  {:result => ($PRGS.where(:uuid => uuid).all)}.to_json
end

# prg creation
post '/api/fungals' do
  uuid = SecureRandom.base64(10)

  if(not params["prg"].nil? and params["prg"].length > 0)
    prg = params["prg"]
    
    $MQ << "#{uuid}#{prg}"
  end

  {:result => {:uuid => uuid}}.to_json
end

# oldest
get "/api/fungals/oldest" do
  {:result => ($PRGS.min(:created_at).limit(10).all)}.to_json
end
