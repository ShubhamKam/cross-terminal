package com.crossplatform.terminal.terminal

import android.view.KeyEvent
import android.view.inputmethod.BaseInputConnection
import android.view.inputmethod.EditorInfo

/**
 * Custom input connection for handling soft keyboard input
 * 
 * Handles text input, key events, and IME operations for the terminal view
 * Provides proper integration with Android's input method framework
 */
class TerminalInputConnection(
    private val terminalView: TerminalView,
    fullEditor: Boolean
) : BaseInputConnection(terminalView, fullEditor) {
    
    override fun commitText(text: CharSequence?, newCursorPosition: Int): Boolean {
        if (text != null && text.isNotEmpty()) {
            terminalView.terminalController?.sendInput(text.toString())
            return true
        }
        return false
    }
    
    override fun sendKeyEvent(event: KeyEvent): Boolean {
        return terminalView.terminalController?.handleKeyInput(event.keyCode, event) ?: false
    }
    
    override fun deleteSurroundingText(beforeLength: Int, afterLength: Int): Boolean {
        if (beforeLength > 0) {
            terminalView.terminalController?.sendInput("\b".repeat(beforeLength))
            return true
        }
        return false
    }
    
    override fun performEditorAction(editorAction: Int): Boolean {
        return when (editorAction) {
            EditorInfo.IME_ACTION_DONE,
            EditorInfo.IME_ACTION_GO,
            EditorInfo.IME_ACTION_SEND -> {
                terminalView.terminalController?.sendInput("\n")
                true
            }
            else -> false
        }
    }
    
    override fun setComposingText(text: CharSequence?, newCursorPosition: Int): Boolean {
        return commitText(text, newCursorPosition)
    }
    
    override fun finishComposingText(): Boolean {
        return true
    }
    
    override fun getTextBeforeCursor(n: Int, flags: Int): CharSequence {
        return ""
    }
    
    override fun getTextAfterCursor(n: Int, flags: Int): CharSequence {
        return ""
    }
}