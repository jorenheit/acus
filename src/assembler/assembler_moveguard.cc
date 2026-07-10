#include "assembler.ih"

Assembler::MoveGuard::MoveGuard(Assembler &a, SlotProxy proxy, bool forceMove) {
  if (proxy.directRelative()) { // direct proxy: materialize to check for temp-ness
    Slot storage = a.materialize(proxy);
    if (forceMove || storage.kind() == Slot::Temp) {
      _oldConsumableStatus = storage.consumable();
      storage.get().allowConsumption(true);
      _guardedSlot = storage;	  
    }
  }
}
    
Assembler::MoveGuard::~MoveGuard() {
  if (not _guardedSlot) return;           // no slot was guarded
  if (_guardedSlot->consumed()) return;   // guarded slot was consumed

  // Guarded slot not consumed -> reset consumable status
  (*_guardedSlot).get().allowConsumption(_oldConsumableStatus);
}
