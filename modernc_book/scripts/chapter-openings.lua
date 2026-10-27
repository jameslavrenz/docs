-- Start each chapter and appendix on a fresh page in the reading edition.
-- This is presentation-only: the authoritative Markdown remains unchanged.
function Header(el)
  if el.level == 1 then
    return {
      pandoc.RawBlock("latex", "\\clearpage"),
      el
    }
  elseif el.level <= 3 then
    -- Reserve space for a heading and at least a few lines of body text.
    return {
      pandoc.RawBlock("latex", "\\Needspace{5\\baselineskip}"),
      el
    }
  end
end
