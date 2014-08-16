/*
 Copyright (C) 2010-2014 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "EntityPropertyCommand.h"

#include "CollectionUtils.h"
#include "Macros.h"
#include "Model/Entity.h"
#include "Model/ModelUtils.h"
#include "View/MapDocument.h"

namespace TrenchBroom {
    namespace Controller {
        const Command::CommandType EntityPropertyCommand::Type = Command::freeType();

        EntityPropertyCommand::PropertySnapshot::PropertySnapshot(const Model::PropertyKey& i_key, const Model::PropertyValue& i_value) :
        key(i_key),
        value(i_value) {}

        void EntityPropertyCommand::setKey(const Model::PropertyKey& key) {
            m_oldKey = key;
        }
        
        void EntityPropertyCommand::setNewKey(const Model::PropertyKey& newKey) {
            m_newKey = newKey;
        }
        
        void EntityPropertyCommand::setNewValue(const Model::PropertyValue& newValue) {
            m_newValue = newValue;
        }
        
        EntityPropertyCommand::EntityPropertyCommand(View::MapDocumentWPtr document, const Action command, const Model::EntityList& entities, const bool force) :
        DocumentCommand(Type, makeName(command), true, document),
        m_action(command),
        m_entities(entities),
        m_force(force) {}

        Command::Ptr EntityPropertyCommand::renameEntityProperty(View::MapDocumentWPtr document, const Model::EntityList& entities, const Model::PropertyKey& oldKey, const Model::PropertyKey& newKey, const bool force) {
            EntityPropertyCommand::Ptr command(new EntityPropertyCommand(document, Action_Rename, entities, force));
            command->setKey(oldKey);
            command->setNewKey(newKey);
            return command;
        }
        
        Command::Ptr EntityPropertyCommand::setEntityProperty(View::MapDocumentWPtr document, const Model::EntityList& entities, const Model::PropertyKey& key, const Model::PropertyKey& newValue, const bool force) {
            EntityPropertyCommand::Ptr command(new EntityPropertyCommand(document, Action_Set, entities, force));
            command->setKey(key);
            command->setNewValue(newValue);
            return command;
        }
        
        Command::Ptr EntityPropertyCommand::removeEntityProperty(View::MapDocumentWPtr document, const Model::EntityList& entities, const Model::PropertyKey& key, const bool force) {
            EntityPropertyCommand::Ptr command(new EntityPropertyCommand(document, Action_Remove, entities, force));
            command->setKey(key);
            return command;
        }

        const Model::PropertyKey& EntityPropertyCommand::key() const {
            return m_oldKey;
        }
        
        const Model::PropertyKey& EntityPropertyCommand::newKey() const {
            return m_newKey;
        }
        
        const Model::PropertyValue& EntityPropertyCommand::newValue() const {
            return m_newValue;
        }

        bool EntityPropertyCommand::definitionAffected() const {
            return m_definitionAffected;
        }
        
        bool EntityPropertyCommand::propertyAffected(const Model::PropertyKey& key) {
            return m_newKey == key || m_oldKey == key;
        }
        
        bool EntityPropertyCommand::entityAffected(const Model::Entity* entity) {
            return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
        }
        
        const Model::EntityList& EntityPropertyCommand::affectedEntities() const {
            return m_entities;
        }

        String EntityPropertyCommand::makeName(const Action command) {
            switch (command) {
                case Action_Rename:
                    return "Rename entity property";
                case Action_Set:
                    return "Set entity property";
                case Action_Remove:
                    return "Remove entity property";
                DEFAULT_SWITCH()
            }
        }

        bool EntityPropertyCommand::doPerformDo() {
            if (!m_force && affectsImmutablePropertyValue())
                return false;
            if (m_action == Action_Rename)
                if (!canSetKey() || (!m_force && affectsImmutablePropertyKey()))
                    return false;
            
            View::MapDocumentSPtr document = lockDocument();
            m_snapshot.clear();
            
            const Model::ObjectList objects = Model::makeObjectList(m_entities);
            document->objectsWillChangeNotifier(objects);
            switch (m_action) {
                case Action_Rename:
                    doRename(document);
                    break;
                case Action_Set:
                    doSetValue(document);
                    break;
                case Action_Remove:
                    doRemove(document);
                    break;
            }
            document->objectsDidChangeNotifier(objects);
            
            return true;
        }
        
        bool EntityPropertyCommand::doPerformUndo() {
            View::MapDocumentSPtr document = lockDocument();
            
            const Model::ObjectList objects = Model::makeObjectList(m_entities);
            document->objectsWillChangeNotifier(objects);
            switch(m_action) {
                case Action_Rename:
                    undoRename(document);
                    break;
                case Action_Set:
                    undoSetValue(document);
                    break;
                case Action_Remove:
                    undoRemove(document);
                    break;
            };
            document->objectsDidChangeNotifier(objects);
            m_snapshot.clear();
            
            return true;
        }

        bool EntityPropertyCommand::doIsRepeatable(View::MapDocumentSPtr document) const {
            return document->hasSelectedEntities();
        }
        
        Command* EntityPropertyCommand::doRepeat(View::MapDocumentSPtr document) const {
            const Model::EntityList& entities = document->allSelectedEntities();
            return new EntityPropertyCommand(document, m_action, entities, m_force);
        }

        bool EntityPropertyCommand::doCollateWith(Command::Ptr command) {
            const Ptr other = command->cast<EntityPropertyCommand>(command);
            
            if (other->m_action != m_action ||
                other->m_force != m_force ||
                other->m_oldKey != m_oldKey ||
                other->m_newKey != m_newKey)
                return false;
            
            m_newValue = other->m_newValue;
            return true;
        }

        void EntityPropertyCommand::doRename(View::MapDocumentSPtr document) {
            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                if (entity->hasProperty(key())) {
                    const Model::PropertyValue& value = entity->property(key());
                    m_snapshot.insert(std::make_pair(entity, PropertySnapshot(key(), value)));
                    
                    entity->renameProperty(key(), newKey());
                    document->entityPropertyDidChangeNotifier(entity, key(), value, newKey(), value);
                }
            }
        }
        
        void EntityPropertyCommand::doSetValue(View::MapDocumentSPtr document) {
            m_definitionAffected = (key() == Model::PropertyKeys::Classname);
            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                if (entity->hasProperty(key())) {
                    const Model::PropertyValue& oldValue = entity->property(key());
                    
                    m_snapshot.insert(std::make_pair(entity, PropertySnapshot(key(), oldValue)));
                    entity->addOrUpdateProperty(key(), newValue());
                    document->entityPropertyDidChangeNotifier(entity, key(), oldValue, key(), newValue());
                } else {
                    entity->addOrUpdateProperty(key(), newValue());
                    document->entityPropertyDidChangeNotifier(entity, "", "", key(), newValue());
                }
            }
        }
        
        void EntityPropertyCommand::doRemove(View::MapDocumentSPtr document) {
            const Model::EntityProperty empty;

            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                if (entity->hasProperty(key())) {
                    const Model::PropertyValue& oldValue = entity->property(key());

                    m_snapshot.insert(std::make_pair(entity, PropertySnapshot(key(), oldValue)));
                    entity->removeProperty(key());
                    document->entityPropertyDidChangeNotifier(entity, key(), oldValue, "", "");
                }
            }
        }

        void EntityPropertyCommand::undoRename(View::MapDocumentSPtr document) {
            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                PropertySnapshotMap::iterator snapshotIt = m_snapshot.find(entity);
                if (snapshotIt != m_snapshot.end()) {
                    const Model::PropertyValue& value = snapshotIt->second.value;
                    
                    entity->renameProperty(newKey(), key());
                    document->entityPropertyDidChangeNotifier(entity, newKey(), value, key(), value);
                }
            }
        }
        
        void EntityPropertyCommand::undoSetValue(View::MapDocumentSPtr document) {
            // const Model::EntityProperty empty;
            // const Model::EntityProperty after(key(), newValue());

            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                PropertySnapshotMap::iterator snapshotIt = m_snapshot.find(entity);
                if (snapshotIt == m_snapshot.end()) {
                    entity->removeProperty(key());
                    document->entityPropertyDidChangeNotifier(entity, key(), newValue(), "", "");
                } else {
                    const PropertySnapshot& before = snapshotIt->second;
                    entity->addOrUpdateProperty(before.key, before.value);
                    document->entityPropertyDidChangeNotifier(entity, key(), newValue(), before.key, before.value);
                }
            }
        }
        
        void EntityPropertyCommand::undoRemove(View::MapDocumentSPtr document) {
            const Model::EntityProperty empty;

            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity* entity = *entityIt;
                PropertySnapshotMap::iterator snapshotIt = m_snapshot.find(entity);
                if (snapshotIt != m_snapshot.end()) {
                    const PropertySnapshot& snapshot = snapshotIt->second;
                    entity->addOrUpdateProperty(snapshot.key, snapshot.value);
                    document->entityPropertyDidChangeNotifier(entity, "", "", snapshot.key, snapshot.value);
                }
            }
        }

        bool EntityPropertyCommand::affectsImmutablePropertyKey() const {
            return (!Model::isPropertyKeyMutable(newKey()) ||
                    !Model::isPropertyKeyMutable(key()));
        }
        
        bool EntityPropertyCommand::affectsImmutablePropertyValue() const {
            return !Model::isPropertyValueMutable(key());
        }

        bool EntityPropertyCommand::canSetKey() const {
            return (key() != m_newKey &&
                    !anyEntityHasProperty(newKey()));
        }

        bool EntityPropertyCommand::anyEntityHasProperty(const Model::PropertyKey& key) const {
            Model::EntityList::const_iterator entityIt, entityEnd;
            for (entityIt = m_entities.begin(), entityEnd = m_entities.end(); entityIt != entityEnd; ++entityIt) {
                Model::Entity& entity = **entityIt;
                if (entity.hasProperty(key))
                    return true;
            }
            return false;
        }
    }
}
