require 'sequel'

Sequel.migration do
  up do
    create_table :prgs do
      primary_key :id
      String :uuid, :null => false
      
      DateTime :created_at
      DateTime :died_at
    end
  end

  down do
    drop_table :prgs
  end
end
