package com.example.realms
import org.junit.Assert.*
import org.junit.Test
class ToolStateTest {
    @Test fun emptyHandsHaveNoActions() {
        val s=ToolState()
        assertEquals(HandTool.HAND_EMPTY,s.tool)
        assertFalse(s.spawnVisible || s.gravityVisible || s.deleteVisible || s.rotateHeldVisible)
    }
    @Test fun toolSwitchHidesOldControls() {
        val s=ToolState()
        for(tool in HandTool.values()) {
            s.select(tool)
            assertEquals(tool==HandTool.SPAWN_GUN,s.spawnVisible)
            assertEquals(tool==HandTool.GRAVITY_GUN,s.gravityVisible)
            assertEquals(tool==HandTool.SPAWN_GUN,s.deleteVisible)
            assertEquals(tool==HandTool.SPAWN_GUN || tool==HandTool.GRAVITY_GUN,s.viewmodelVisible)
        }
    }
    @Test fun cycleIsImmediateAndSkipsDeleteTool() {
        val s=ToolState()
        repeat(4) {
            s.select(s.nextTool());assertEquals(HandTool.SPAWN_GUN,s.tool);assertTrue(s.deleteVisible)
            s.select(s.nextTool());assertEquals(HandTool.GRAVITY_GUN,s.tool);assertFalse(s.deleteVisible)
            s.confirmHolding(true)
            s.select(s.nextTool());assertEquals(HandTool.HAND_EMPTY,s.tool);assertFalse(s.holding)
        }
    }
    @Test fun captureOnlyChangesOnWorldConfirmation() {
        val s=ToolState();s.select(HandTool.GRAVITY_GUN)
        assertEquals("Захват",s.grabCaption);assertFalse(s.rotateHeldVisible)
        s.confirmHolding(true);assertEquals("Бросить",s.grabCaption);assertTrue(s.rotateHeldVisible)
        s.select(HandTool.SPAWN_GUN);assertFalse(s.holding);assertFalse(s.rotateHeldVisible)
        s.confirmHolding(true);assertFalse(s.holding)
        s.select(HandTool.GRAVITY_GUN);s.confirmHolding(true);s.confirmHolding(false)
        assertEquals("Захват",s.grabCaption)
    }
}
