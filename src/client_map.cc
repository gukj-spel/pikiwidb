#include "client_map.h"

namespace pikiwidb {

uint32_t ClientMap::GetAllClientInfos(std::vector<ClientInfo>& results) {
  // client info string type: ip, port, fd.
  std::shared_lock<std::shared_mutex> client_map_lock(client_map_mutex_);
  auto it = clients_.begin();
  while (it != clients_.end()) {
    auto client = it->second.lock();
    if (client) {
      results.emplace_back(client->GetClientInfo());
    }
    it++;
  }
  return results.size();
}

bool ClientMap::AddClient(int id, std::weak_ptr<PClient> client) {
  std::unique_lock client_map_lock(client_map_mutex_);
  if (clients_.find(id) == clients_.end()) {
    clients_.insert({id, client});
    return true;
  }
  return false;
}

ClientInfo ClientMap::GetClientsInfoById(int id) {
  std::shared_lock client_map_lock(client_map_mutex_);
  if (auto it = clients_.find(id); it != clients_.end()) {
    if (auto client = it->second.lock(); client) {
      return client->GetClientInfo();
    }
  }
  return ClientInfo::invalidClientInfo;
}

bool ClientMap::RemoveClientById(int id) {
  std::unique_lock client_map_lock(client_map_mutex_);
  if (auto it = clients_.find(id); it != clients_.end()) {
    clients_.erase(it);
    return true;
  }
  return false;
}

bool ClientMap::KillAllClients() {
  std::shared_lock<std::shared_mutex> client_map_lock(client_map_mutex_);
  auto it = clients_.begin();
  while (it != clients_.end()) {
    auto client = it->second.lock();
    if (client) {
      client_map_lock.unlock();
      client->Close();
      client_map_lock.lock();
    }
    it++;
  }
  return true;
}

bool ClientMap::KillClientByAddrPort(const std::string& addr_port) {
  std::shared_lock<std::shared_mutex> client_map_lock(client_map_mutex_);
  for (auto& [id, client_weak] : clients_) {
    auto client = client_weak.lock();
    if (client) {
      std::string client_ip_port = client->PeerIP() + ":" + std::to_string(client->PeerPort());
      if (client_ip_port == addr_port) {
        client_map_lock.unlock();
        client->Close();
        return true;
      }
    }
  }
  return false;
}

bool ClientMap::KillClientById(int client_id) {
  std::shared_lock<std::shared_mutex> client_map_lock(client_map_mutex_);
  if (auto it = clients_.find(client_id); it != clients_.end()) {
    auto client = it->second.lock();
    if (client) {
      client_map_lock.unlock();
      client->Close();
      return true;
    }
  }
  return false;
}

}  // namespace pikiwidb