class Object
    def persist_id
        unless @persist_id
            @@next_object_id ||= 0
            @@objects_persisted ||= { }
            @@objects_persisted[@@next_object_id] = self
            @persist_id = @@next_object_id
            @@next_object_id += 1
        end

        @persist_id
    end

    def unpersist
        if @persist_id
            @@objects_persisted.delete(@persist_id)
        end
    end

    def self.from_persist_id(id)
        @@objects_persisted[id]
    end
end
